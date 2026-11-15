param(
    [string]$GccPath = 'C:\Users\maco\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin\gcc.exe',
    [string]$OutputPath = (Join-Path $PSScriptRoot 'smolnes-v2.exe'),
    [switch]$FrameDiagnostics,
    [switch]$AudioDiagnostics,
    [switch]$ApuTrace
)

$ErrorActionPreference = 'Stop'
if (-not (Test-Path -LiteralPath $GccPath -PathType Leaf)) {
    throw "WinLibs GCC not found: $GccPath"
}
$outputFullPath = [IO.Path]::GetFullPath($OutputPath)
New-Item -ItemType Directory -Path (Split-Path -Parent $outputFullPath) -Force | Out-Null
$gccArgs = @(
    '-Os', '-DNDEBUG', '-ffunction-sections',
    '-static', '-mwindows', '-Wl,--gc-sections', '-s'
)
if ($FrameDiagnostics) {
    $gccArgs += '-DSMOLNES_FRAME_TIME_DIAGNOSTICS'
}
if ($AudioDiagnostics) {
    $gccArgs += '-DSMOLNES_AUDIO_DIAGNOSTICS'
}
if ($ApuTrace) {
    $gccArgs += '-DSMOLNES_APU_TRACE'
}
$gccArgs += @(
    (Join-Path $PSScriptRoot 'deobfuscated_win32.c'),
    (Join-Path $PSScriptRoot 'apu.c'),
    (Join-Path $PSScriptRoot 'audio_win32.c'),
    '-o', $outputFullPath, '-luser32', '-lgdi32', '-lkernel32', '-lwinmm'
)
& $GccPath @gccArgs
if ($LASTEXITCODE -ne 0) {
    throw "GCC failed with exit code $LASTEXITCODE."
}
$compilerVersion = (& $GccPath --version | Select-Object -First 1)
$hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $outputFullPath).Hash
Write-Output "Compiler: $compilerVersion"
Write-Output "Built: $outputFullPath"
Write-Output "Size: $((Get-Item -LiteralPath $outputFullPath).Length) bytes"
Write-Output "SHA256: $hash"

