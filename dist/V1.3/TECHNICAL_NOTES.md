# smolnes V1.3 — minimum-size Windows build

V1.3 aims to produce a very small, usable Windows FC/NES emulator executable while retaining the current emulator features. The four release files use the same emulator source and ROM-loading flow; they differ only by CPU architecture and executable packing.

## How to run

Double-click an EXE. It opens the standard Windows file picker. Choose a `.nes` ROM and click **Open**; the emulator window then starts. The program intentionally does not accept a ROM filename as a command-line argument. Canceling the picker exits. To choose another ROM, close and relaunch the program.

| File | Intended system | On-disk size | Notes |
| --- | --- | ---: | --- |
| `smolnes-v1.3-native-caption-x64.exe` | 64-bit Windows | 10,240 B | Uncompressed x64 build |
| `smolnes-v1.3-native-caption-x64-upx.exe` | 64-bit Windows | 8,192 B | Same x64 build, packed with UPX `--ultra-brute` |
| `smolnes-v1.3-native-caption-x86.exe` | 32-bit or 64-bit Windows | 9,728 B | Uncompressed x86 fixed-base build |
| `smolnes-v1.3-native-caption-x86-upack.exe` | 32-bit or 64-bit Windows | 6,204 B | Same x86 build, packed with UPack |

Packed builds save download and disk space. For easier diagnosis of startup or antivirus issues, use the corresponding uncompressed build.

## Size-reduction methods

- **Windows APIs directly:** the port uses Win32 file I/O, window, timer, input, and GDI interfaces instead of SDL. ROM selection uses the Windows common file dialog (`GetOpenFileNameA`), and loading uses `CreateFileA` / `ReadFile` / `CloseHandle`.
- **Custom process entry:** the program starts at `mainCRTStartup` and links with `-nostartfiles -nostdlib`, avoiding the normal MinGW C runtime startup and default library set. It links only the Windows import libraries and the compiler support library needed by the build.
- **Size-oriented optimization:** GCC uses `-Oz` and link-time optimization (`-flto`), with `-DNDEBUG`, function-level sections, and `--gc-sections` so unused code can be removed.
- **Less metadata:** the release build strips symbols (`-s`) and disables unwind tables. Error-checking paths are compiled out with `SMOLNES_NO_ERROR_CHECKS`.
- **Compact PE layout:** a custom linker script puts the file-backed imports, code, constants, and initialized writable data into one `.idata` section. This reduces per-section headers and alignment padding; the section is marked readable, writable, and executable to preserve the mixed contents.
- **x86 relocation removal:** the x86 executable disables dynamic base and omits `.reloc`, producing a fixed-base image. Windows may need to relocate it if its preferred address is unavailable; this compact build has no relocation table to support that, so x86 compatibility is less flexible.
- **Release-only packing:** UPX `--ultra-brute` is used for x64; UPack is used for x86. Packing changes the distribution file, not the normal build inputs. The uncompressed binaries remain in the package as reproducible comparison and troubleshooting artifacts.

## Build and artifact checks

Both architectures were rebuilt from the included `source/deobfuscated_win32.c` with the included `build-both.ps1` and linker scripts. The x64 packed image passed `upx -t`. The exact artifact hashes are recorded in `RESULTS.md`. Runtime ROM/gameplay validation is not claimed for this release publication.

V1.3's GUI file picker is a special case. Future versions should retain command-line ROM loading so automated smoke and regression tests can start the emulator predictably.

The build snapshots include source, compiler flags, linker scripts, object files, maps, and build metadata so the size can be audited and the uncompressed executables rebuilt. Historical comparison results are in `HISTORICAL_SIZE_RESULTS.md`.
