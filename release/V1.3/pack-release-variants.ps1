param(
    [Parameter(Mandatory = $true)]
    [string]$X86Exe,

    [Parameter(Mandatory = $true)]
    [string]$X64Exe,

    [string]$UpackPath = '',
    [string]$UpxPath = 'C:\Windows\upx.exe'
)

$ErrorActionPreference = 'Stop'

if (-not $UpackPath) {
    $upackCandidates = @(
        (Join-Path $PSScriptRoot '..\tools\Upack.exe'),
        (Join-Path $PSScriptRoot 'Upack.exe')
    )
    $UpackPath = $upackCandidates |
        Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } |
        Select-Object -First 1
    if (-not $UpackPath) {
        throw 'Could not locate Upack.exe. Pass -UpackPath explicitly.'
    }
}
$UpackPath = [IO.Path]::GetFullPath($UpackPath)
$UpxPath = [IO.Path]::GetFullPath($UpxPath)

function Get-PackedPath([string]$Path, [string]$Suffix) {
    $fullPath = [IO.Path]::GetFullPath($Path)
    $directory = [IO.Path]::GetDirectoryName($fullPath)
    $name = [IO.Path]::GetFileNameWithoutExtension($fullPath)
    return Join-Path $directory ($name + '-' + $Suffix + '.exe')
}

function Format-Artifact([string]$Label, [string]$Path) {
    $file = Get-Item -LiteralPath $Path
    $hash = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
    Write-Output ("{0}: {1} bytes  {2}" -f $Label, $file.Length, $file.FullName)
    Write-Output ("  SHA256: {0}" -f $hash)
}

$x86 = [IO.Path]::GetFullPath($X86Exe)
$x64 = [IO.Path]::GetFullPath($X64Exe)
foreach ($inputPath in @($x86, $x64)) {
    if (-not (Test-Path -LiteralPath $inputPath -PathType Leaf)) {
        throw "Input executable not found: $inputPath"
    }
}
foreach ($toolPath in @($UpackPath, $UpxPath)) {
    if (-not (Test-Path -LiteralPath $toolPath -PathType Leaf)) {
        throw "Packing tool not found: $toolPath"
    }
}

$x86Packed = Get-PackedPath $x86 'upack'
$x64Packed = Get-PackedPath $x64 'upx'
if ($x86Packed -eq $x86 -or $x64Packed -eq $x64) {
    throw 'Packed output path must differ from the uncompressed executable.'
}

# Both packers modify their input file. Pack copies so the original build
# artifacts remain available for development, debugging, and comparisons.
Copy-Item -LiteralPath $x86 -Destination $x86Packed -Force
& $UpackPath $x86Packed
if ($LASTEXITCODE -ne 0) {
    Remove-Item -LiteralPath $x86Packed -Force -ErrorAction SilentlyContinue
    throw "UPack failed for x86 executable (exit code $LASTEXITCODE)."
}

Copy-Item -LiteralPath $x64 -Destination $x64Packed -Force
& $UpxPath '--ultra-brute' $x64Packed
if ($LASTEXITCODE -ne 0) {
    Remove-Item -LiteralPath $x64Packed -Force -ErrorAction SilentlyContinue
    throw "UPX --ultra-brute failed for x64 executable (exit code $LASTEXITCODE)."
}
& $UpxPath '-t' $x64Packed
if ($LASTEXITCODE -ne 0) {
    throw "UPX verification failed for $x64Packed (exit code $LASTEXITCODE)."
}

Format-Artifact 'x86 original' $x86
Format-Artifact 'x86 UPack' $x86Packed
Format-Artifact 'x64 original' $x64
Format-Artifact 'x64 UPX' $x64Packed
