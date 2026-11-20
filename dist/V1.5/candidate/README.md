# smolnes V1.5 candidate

V1.5 carries the V1.4 dual-timer implementation and the corrected scanline rollback.

## Changes from V1.4

- Retains the per-scanline OAM scan that selects the first eight in-range sprites from 64 entries.
- Removes the 8-pixel sprite-row predecode and per-pixel sprite result arrays. Pixels are resolved against the selected sprites on demand.
- Removes the background color and palette scanline caches; background values are read directly from the PPU shift registers.
- Removes the `WM_NCCREATE`, Escape `WM_KEYDOWN`, and F4 `WM_SYSKEYDOWN` cases. Escape no longer closes the emulator. Alt+F4 is passed to `DefWindowProcA`, which handles the normal close path. `WM_NCCREATE` is also left to the default window procedure.

## Build

Build both architectures with the included linker scripts and the WinLibs GCC 16.2.0 toolchains:

```powershell
.\build-v1.5.ps1 -GccPathX64 "C:\path\to\x64-gcc.exe" -GccPathX86 "D:\path\to\i686-w64-mingw32-gcc.exe"
```

Build flags are recorded in each executable's `.build-info.txt`. The build produces uncompressed x64 and x86 executables in `release/`, with matching object files and linker maps under `build-support/`.

## Release executables

| Architecture | Variant | Size |
|---|---|---:|
| x64 | Uncompressed | 9,728 B |
| x64 | UPX 5.3.0 (`--brute`) | 8,192 B |
| x86 | Uncompressed | 9,216 B |
| x86 | UPack 0.39 | 5,884 B |

The x86 UPack build was generated from a copy of the uncompressed executable. UPack 0.39 identifies itself as freeware for non-commercial use and provides no warranty; the x86 packed asset carries that restriction. UPX `-t` passed for the packed x64 asset. UPack has no equivalent integrity-check option in its displayed interface.

Builds completed, but runtime/gameplay tests were not run for this candidate.
