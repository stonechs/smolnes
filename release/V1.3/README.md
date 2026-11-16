# smolnes V1.3 final

This directory contains the V1.3 source snapshot, x86/x64 build and linker scripts, object files, maps, and build metadata. The four executable files are attached to the GitHub Release.

## Running the emulator

For step-by-step instructions in Chinese, see [`使用说明.md`](使用说明.md). In short: double-click an EXE, select a `.nes` ROM in the Windows file dialog, then choose **Open**. V1.3 does not accept the ROM path as a command-line argument.

## Choose a build

| Architecture | Uncompressed | Compressed |
| --- | --- | --- |
| x64 | `smolnes-v1.3-native-caption-x64.exe` | `smolnes-v1.3-native-caption-x64-upx.exe` |
| x86 | `smolnes-v1.3-native-caption-x86.exe` | `smolnes-v1.3-native-caption-x86-upack.exe` |

For ordinary use, the compressed builds are smallest. The unpacked x64 build is the most straightforward choice for compatibility and troubleshooting. The x86 UPack build may trigger warnings from some antivirus products because it is compressed with an executable packer.

See [`TECHNICAL_NOTES.md`](TECHNICAL_NOTES.md) for the size-reduction methods and artifact checks. Rebuild from this directory with `build-both.ps1`; regenerate the packed copies with `pack-release-variants.ps1`. Historical size comparisons are in [`HISTORICAL_SIZE_RESULTS.md`](HISTORICAL_SIZE_RESULTS.md).
