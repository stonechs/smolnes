param(
    [string]$GccPath = 'C:\Users\maco\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin\gcc.exe',
    [string]$UpxPath = 'C:\Windows\upx.exe',
    [string]$OutputPath = (Join-Path $PSScriptRoot 'smolnes-v3.exe'),
    [string]$MapPath = ''
)
$ErrorActionPreference = 'Stop'
if (!(Test-Path -LiteralPath $GccPath -PathType Leaf)) { throw "GCC not found: $GccPath" }
if (!(Test-Path -LiteralPath $UpxPath -PathType Leaf)) { throw "UPX not found: $UpxPath" }
$output = [IO.Path]::GetFullPath($OutputPath)
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $output) | Out-Null
$raw = $output + '.unpacked'
$src = Join-Path $PSScriptRoot 'source'
$args = @('-Os','-flto','-DNDEBUG','-ffunction-sections','-nostartfiles','-nostdlib','-static','-mwindows','-Wl,--gc-sections','-s','-Wl,--no-insert-timestamp','-Wl,-e,mainCRTStartup')
$args += @((Join-Path $src 'deobfuscated_win32.c'),(Join-Path $src 'apu.c'),(Join-Path $src 'audio_win32.c'),'-o',$raw,'-luser32','-lgdi32','-lkernel32','-lwinmm','-lgcc')
if ($MapPath) { $args += '-Wl,-Map=' + [IO.Path]::GetFullPath($MapPath) }
& $GccPath @args
if ($LASTEXITCODE -ne 0) { throw "GCC failed: $LASTEXITCODE" }
& $UpxPath '--best' '--lzma' $raw
if ($LASTEXITCODE -ne 0) { throw "UPX failed: $LASTEXITCODE" }
& $UpxPath '-t' $raw
if ($LASTEXITCODE -ne 0) { throw "UPX verification failed: $LASTEXITCODE" }
if (Test-Path -LiteralPath $output) { Remove-Item -LiteralPath $output -Force }
Move-Item -LiteralPath $raw -Destination $output
$compiler = & $GccPath --version | Select-Object -First 1
Write-Output "Compiler: $compiler"
Write-Output "UPX: $(& $UpxPath --version | Select-Object -First 1)"
Write-Output "Built packed executable: $output"
Write-Output "Size: $((Get-Item -LiteralPath $output).Length) bytes"
Write-Output "SHA256: $((Get-FileHash -LiteralPath $output -Algorithm SHA256).Hash)"
