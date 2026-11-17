param(
    [string]$GccPathX64 = 'gcc.exe',
    [string]$GccPathX86 = 'i686-w64-mingw32-gcc.exe'
)

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$source = Join-Path $root 'source\deobfuscated_win32.c'
$release = Join-Path $root 'release'
$objects = Join-Path $root 'build-support\objects'
$maps = Join-Path $root 'build-support\maps'
New-Item -ItemType Directory -Force -Path $release, $objects, $maps | Out-Null

function Resolve-Gcc([string]$Path) {
    if (Test-Path -LiteralPath $Path -PathType Leaf) {
        return (Resolve-Path -LiteralPath $Path).Path
    }
    return (Get-Command $Path -ErrorAction Stop).Source
}

$targets = @(
    @{ Arch = 'x64'; Gcc = $GccPathX64; Entry = 'mainCRTStartup' },
    @{ Arch = 'x86'; Gcc = $GccPathX86; Entry = '_mainCRTStartup' }
)
$common = @('-DSMOLNES_OMIT_TIMER_FAILURE_CHECKS', '-DSMOLNES_OMIT_TIMER_SHUTDOWN_CLEANUP', '-Oz', '-flto', '-DNDEBUG', '-ffunction-sections', '-fno-unwind-tables', '-fno-asynchronous-unwind-tables', '-DSMOLNES_NO_ERROR_CHECKS')

foreach ($target in $targets) {
    $arch = $target.Arch
    $gcc = Resolve-Gcc $target.Gcc
    $object = Join-Path $objects "deobfuscated_win32-$arch.o"
    $exe = Join-Path $release "smolnes-v1.4-dual-timer-$arch.exe"
    $map = Join-Path $maps "link-$arch.map"
    $linker = Join-Path $root "build-support\linker\linker-merge-one-section-$arch.ld"
    $entry = $target.Entry

    & $gcc @common '-c' $source '-o' $object
    if ($LASTEXITCODE -ne 0) { throw "Object compile failed: $arch" }

    $linkArgs = @($common) + @('-static', '-mwindows', '-Wl,--gc-sections', '-s', '-Wl,--no-insert-timestamp', '-nostartfiles', "-Wl,-e,$entry", '-nostdlib', "-Wl,-T,$linker")
    if ($arch -eq 'x86') { $linkArgs += @('-Wl,--disable-dynamicbase', '-Wl,--disable-reloc-section') }
    $linkArgs += @($object, '-o', $exe, "-Wl,-Map=$map", '-luser32', '-lgdi32', '-lkernel32', '-lcomdlg32', '-lgcc')

    & $gcc @linkArgs
    if ($LASTEXITCODE -ne 0) { throw "Link failed: $arch" }

    $bytes = [IO.File]::ReadAllBytes($exe)
    $pe = [BitConverter]::ToInt32($bytes, 0x3c)
    if ([Text.Encoding]::ASCII.GetString($bytes, $pe, 4) -ne "PE`0`0") { throw 'Invalid PE signature' }
    $count = [BitConverter]::ToUInt16($bytes, $pe + 6)
    $optional = [BitConverter]::ToUInt16($bytes, $pe + 20)
    $headers = $pe + 24 + $optional
    $found = $false
    for ($i = 0; $i -lt $count; $i++) {
        $section = $headers + 40 * $i
        $name = [Text.Encoding]::ASCII.GetString($bytes, $section, 8).Trim([char]0)
        if ($name -eq '.idata') {
            $field = $section + 36
            $flags = [BitConverter]::ToUInt32($bytes, $field) -bor 0xA0000020
            [Array]::Copy([BitConverter]::GetBytes($flags), 0, $bytes, $field, 4)
            $found = $true
            break
        }
    }
    if (!$found) { throw 'Merged .idata section was not produced' }
    [IO.File]::WriteAllBytes($exe, $bytes)

    $hash = (Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash
    $metadata = @(
        'Version: V1.4 dual-timer release build',
        'Variant: extreme timer-size implementation',
        "Architecture: $arch",
        "Compiler: $(& $gcc --version | Select-Object -First 1)",
        "Target: $(& $gcc -dumpmachine | Select-Object -First 1)",
        'Source: source/deobfuscated_win32.c',
        "Source SHA256: $((Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash)",
        "Compile flags: $($common -join ' ')",
        "Linker script: build-support/linker/linker-merge-one-section-$arch.ld",
        "Entry point: $entry",
        'Link flags: -static -mwindows --gc-sections -s --no-insert-timestamp -nostartfiles -nostdlib -luser32 -lgdi32 -lkernel32 -lcomdlg32 -lgcc',
        'PE layout: single RWX .idata file-backed section plus zero-fill .bss',
        'Timer creation/start failures are unchecked; timer shutdown cleanup is omitted',
        "Object bytes: $((Get-Item -LiteralPath $object).Length)",
        "EXE bytes: $((Get-Item -LiteralPath $exe).Length)",
        "SHA256: $hash"
    )
    $metadata | Set-Content -LiteralPath "$exe.build-info.txt" -Encoding utf8
}
