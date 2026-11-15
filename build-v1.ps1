param(
    [string]$GccPath = 'gcc.exe',
    [string]$OutputPath = (Join-Path $PSScriptRoot 'dist\V1\smolnes-v1.exe')
)

$ErrorActionPreference = 'Stop'
$sourcePath = Join-Path $PSScriptRoot 'deobfuscated_win32.c'

if (Test-Path -LiteralPath $GccPath -PathType Leaf) {
    $compilerPath = (Resolve-Path -LiteralPath $GccPath).Path
} else {
    $compiler = Get-Command $GccPath -ErrorAction SilentlyContinue
    if (-not $compiler) {
        throw "GCC not found. Pass -GccPath with the full path to gcc.exe."
    }
    $compilerPath = $compiler.Source
}

if (-not (Test-Path -LiteralPath $sourcePath -PathType Leaf)) {
    throw "V1 source not found: $sourcePath"
}

$outputFullPath = [IO.Path]::GetFullPath($OutputPath)
$outputDirectory = Split-Path -Parent $outputFullPath
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null

$gccArgs = @(
    '-O3', '-DNDEBUG', '-Os', '-ffunction-sections',
    '-static', '-mwindows', '-Wl,--gc-sections', '-s',
    $sourcePath, '-o', $outputFullPath,
    '-luser32', '-lgdi32', '-lkernel32'
)

& $compilerPath @gccArgs
if ($LASTEXITCODE -ne 0) {
    throw "GCC failed with exit code $LASTEXITCODE."
}

$binary = Get-Item -LiteralPath $outputFullPath
$hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $outputFullPath).Hash
Write-Output "Built: $($binary.FullName)"
Write-Output "Size: $($binary.Length) bytes"
Write-Output "SHA256: $hash"
