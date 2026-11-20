# smolnes Windows-native V1.5

V1.5 follows the V1.4 dual-timer build. The release contains four x64/x86 executables, packed and uncompressed.

## Changes

- Retains selecting the first eight in-range sprites from the 64 OAM entries once per scanline.
- Removes the 8-pixel sprite-row predecode and resolves each pixel against the selected sprites on demand.
- Removes the background color and palette scanline caches; reads those values directly from the PPU shift registers.
- Removes the explicit `WM_NCCREATE`, Escape `WM_KEYDOWN`, and F4 `WM_SYSKEYDOWN` cases. Escape no longer exits the emulator. Alt+F4 continues through `DefWindowProcA` and the existing `WM_CLOSE` handler.

## ROM loading

At startup, select a `.nes` file in the Windows file picker. This build does not accept a ROM path on the command line.

## Assets

- x64 uncompressed: 9,728 B
- x64 UPX 5.3.0 `--brute`: 8,192 B
- x86 uncompressed: 9,216 B
- x86 UPack 0.39: 5,884 B

UPX `-t` passed for the x64 packed asset. UPack 0.39 states that it is freeware for non-commercial use and provides no warranty; the x86 packed asset carries that restriction. UPack has no equivalent integrity-check option in its displayed interface.

The x64/x86 builds compiled with WinLibs GCC 16.2.0, `-Oz -flto`, dead-section elimination, stripped symbols, and no unwind tables. Runtime and gameplay behavior were not tested for V1.5.
