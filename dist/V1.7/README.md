# smolnes Windows-native V1.7

V1.7 adds optional command-line ROM loading while keeping the Open ROM file dialog. With no ROM argument, the dialog appears. A simple ROM filename can be passed at startup and is opened relative to the current working directory. The lightweight command-line reader does not decode quoted filenames containing spaces.

## Outputs

| Architecture | Output | Size |
|---|---|---:|
| x86 | Clang + LLD, uncompressed | 9,216 B |
| x86 | Clang native COFF + Crinkler 3.0b FAST | 4,565 B |
| x64 | Clang `-Oz -flto` + LLD, uncompressed | 11,264 B |
| x64 | Clang `-Oz -flto` + UPX 5.3.0 `--brute` | 8,192 B |

Total: **33,237 B**. The packed x86 and x64 outputs total **12,757 B**. The x86 linker map reports 6,807 B of `.text` code, 71 B more than V1.6's 6,736 B. The Crinkler output is 41 B larger than V1.6.

## Verification

The final x86 Crinkler binary was tested with no arguments: the Open dialog appeared, the directory was navigated to the executable folder, `pig.NES` was selected, and its game title screen appeared. The same binary also loaded `pig.NES` when passed as the sole simple filename from its working directory. Short CPU samples after loading were 1.5% and 0.4% of a four-logical-processor machine. The window was closed with its title-bar X.

Only x86 Crinkler was runtime-tested. The x86 uncompressed and both x64 outputs were compiled but not separately launched.

Rebuild from the repository root with `scripts/build-v1.7-clang.cmd`. Build objects and maps are under `dist/V1.7/build-support/`; hashes and build details are in `dist/V1.7/release/`.
