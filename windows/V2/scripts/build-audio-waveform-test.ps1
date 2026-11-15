param(
    [string]$GccPath = 'C:\Users\maco\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin\gcc.exe',
    [string]$OutputPath = (Join-Path $PSScriptRoot '..\dist\V2\stage-03\audio-waveform-test.exe')
)

$ErrorActionPreference = 'Stop'
if (-not (Test-Path -LiteralPath $GccPath -PathType Leaf)) {
    throw "WinLibs GCC not found: $GccPath"
}
$outputFullPath = [IO.Path]::GetFullPath($OutputPath)
New-Item -ItemType Directory -Path (Split-Path -Parent $outputFullPath) -Force | Out-Null
& $GccPath -O2 -Wall -Wextra -DAUDIO_TESTING `
    (Join-Path $PSScriptRoot 'audio-waveform-test.c') `
    (Join-Path $PSScriptRoot '..\audio_win32.c') -o $outputFullPath -lwinmm
if ($LASTEXITCODE -ne 0) {
    throw "GCC failed with exit code $LASTEXITCODE."
}
$compilerVersion = (& $GccPath --version | Select-Object -First 1)
$hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $outputFullPath).Hash
Write-Output "Compiler: $compilerVersion"
Write-Output "Built: $outputFullPath"
Write-Output "Size: $((Get-Item -LiteralPath $outputFullPath).Length) bytes"
Write-Output "SHA256: $hash"
