# smolnes Windows-native V1.7

V1.7 retains the ROM file dialog and adds optional command-line ROM loading. No argument opens the dialog; one simple filename opens relative to the current working directory. Quoted filenames containing spaces are not supported by the minimal argument reader.

## Outputs

- x86 Clang/LLD: 9,216 B
- x86 Clang/Crinkler 3.0b FAST: 4,565 B
- x64 Clang/LLD: 11,264 B
- x64 Clang/UPX 5.3.0 `--brute`: 8,192 B

Total: 33,237 B; compressed pair: 12,757 B. The aggregate is 553 B larger than V1.6. The x86 map `.text` code is 6,807 B, up 71 B from V1.6; Crinkler output grew 41 B.

## Verification

The x86 Crinkler binary was tested using both startup modes. The file dialog was navigated to the executable directory and `pig.NES` was selected; the game title screen appeared. Passing `pig.NES` as a command-line argument from the same directory also loaded it. Two one-second CPU samples were low (1.5% and 0.4% of the four-logical-processor machine). The test window was closed using its title-bar X. The other three outputs were built but not separately runtime-tested.

Build script: `scripts/build-v1.7-clang.cmd` (snapshot source); the repository-root build script is also at `../../scripts/build-v1.7-clang.cmd`. SHA-256 values and per-file build details are in `release/`.
