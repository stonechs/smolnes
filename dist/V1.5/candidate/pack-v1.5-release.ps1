param(
    [string]$UpackPath = '',
    [string]$UpxPath = ''
)

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$release = Join-Path $root 'release'

function Resolve-Packer([string]$Path, [string[]]$Candidates, [string]$CommandName) {
    if ($Path) {
        if (!(Test-Path -LiteralPath $Path -PathType Leaf)) { throw "Packer not found: $Path" }
        return (Resolve-Path -LiteralPath $Path).Path
    }
    foreach ($candidate in $Candidates) {
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }
    $command = Get-Command $CommandName -ErrorAction SilentlyContinue
    if ($command) { return $command.Source }
    throw "Could not locate $CommandName. Pass its path explicitly."
}

$tools = Join-Path $root '..\..\..\tools'
$UpackPath = Resolve-Packer $UpackPath @((Join-Path $tools 'Upack.exe')) 'Upack.exe'
$UpxPath = Resolve-Packer $UpxPath @((Join-Path $tools 'upx.exe')) 'upx.exe'
$x86 = Join-Path $release 'smolnes-v1.5-scanline-rollback-x86.exe'
$x64 = Join-Path $release 'smolnes-v1.5-scanline-rollback-x64.exe'
$x86Packed = Join-Path $release 'smolnes-v1.5-scanline-rollback-x86-upack.exe'
$x64Packed = Join-Path $release 'smolnes-v1.5-scanline-rollback-x64-upx.exe'

foreach ($input in @($x86, $x64)) {
    if (!(Test-Path -LiteralPath $input -PathType Leaf)) { throw "Uncompressed V1.5 input missing: $input" }
}

# Both packers modify their input, so pack copies and retain the release originals.
Copy-Item -LiteralPath $x86 -Destination $x86Packed -Force
& $UpackPath $x86Packed
if ($LASTEXITCODE -ne 0) { throw "UPack failed for x86 (exit code $LASTEXITCODE)." }
if (!(Test-Path -LiteralPath $x86Packed -PathType Leaf) -or (Get-Item $x86Packed).Length -eq 0) { throw 'UPack did not produce an x86 output.' }

Copy-Item -LiteralPath $x64 -Destination $x64Packed -Force
& $UpxPath '--brute' $x64Packed
if ($LASTEXITCODE -ne 0) { throw "UPX --brute failed for x64 (exit code $LASTEXITCODE)." }
& $UpxPath '-t' $x64Packed
if ($LASTEXITCODE -ne 0) { throw 'UPX integrity check failed.' }

$upxVersion = (& $UpxPath '--version' | Select-Object -First 1)
$inputs = @(
    @{ Path = $x86Packed; Packer = 'UPack 0.39'; Args = 'in-place on a copy of the x86 release executable'; Input = $x86 },
    @{ Path = $x64Packed; Packer = $upxVersion; Args = '--brute'; Input = $x64 }
)
foreach ($item in $inputs) {
    $inputHash = (Get-FileHash -LiteralPath $item.Input -Algorithm SHA256).Hash
    $outputHash = (Get-FileHash -LiteralPath $item.Path -Algorithm SHA256).Hash
    $file = Get-Item -LiteralPath $item.Path
    @(
        'Version: V1.5 dual-timer release',
        "Packer: $($item.Packer)",
        "Packer arguments: $($item.Args)",
        "Input: $([IO.Path]::GetFileName($item.Input))",
        "Input SHA256: $inputHash",
        "Output bytes: $($file.Length)",
        "Output SHA256: $outputHash"
    ) | Set-Content -LiteralPath "$($item.Path).build-info.txt" -Encoding utf8
    Write-Output "$($file.Name): $($file.Length) bytes; SHA256 $outputHash"
}

$assets = @($x64, $x64Packed, $x86, $x86Packed)
$lines = foreach ($asset in $assets) {
    $hash = (Get-FileHash -LiteralPath $asset -Algorithm SHA256).Hash
    "$hash  $(Split-Path -Leaf $asset)"
}
$lines | Set-Content -LiteralPath (Join-Path $release 'release-assets-sha256.txt') -Encoding utf8
