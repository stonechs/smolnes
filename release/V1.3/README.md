# smolnes V1.3 final

The final V1.3 snapshot is in `final\`. It contains the source, x86/x64 build scripts and linker scripts, object files, maps, build metadata, and all four deliverable executables.

## Running the emulator

For step-by-step instructions in Chinese, see [`final/使用说明.md`](final/使用说明.md). In short: double-click an EXE, select a `.nes` ROM in the Windows file dialog, then choose **Open**. V1.3 does not accept the ROM path as a command-line argument.

## Choose a build

| Architecture | Uncompressed | Compressed |
| --- | --- | --- |
| x64 | `final\smolnes-v1.3-native-caption-x64.exe` | `final\smolnes-v1.3-native-caption-x64-upx.exe` |
| x86 | `final\smolnes-v1.3-native-caption-x86.exe` | `final\smolnes-v1.3-native-caption-x86-upack.exe` |

For ordinary use, the compressed builds are smallest. The unpacked x64 build is the most straightforward choice for compatibility and troubleshooting. The x86 UPack build may trigger warnings from some antivirus products because it is compressed with an executable packer.

See [`final/TECHNICAL_NOTES.md`](final/TECHNICAL_NOTES.md) for the size-reduction methods, tested file sizes, and scope of validation. Rebuild from `final\` with `build-both.ps1`; regenerate the packed copies with `pack-release-variants.ps1`. The previous size analysis is retained as `final\HISTORICAL_SIZE_RESULTS.md`. Earlier V1.3 working files from `dist` are archived under `archive\V1.3-dist-intermediates-2026-11-16\`.
