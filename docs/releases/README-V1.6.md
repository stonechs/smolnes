# smolnes Windows-native V1.6

V1.6 keeps the V1.5 emulator source and uses Clang/LLVM 23.1.5 for both Windows targets. The release includes raw and packed x86/x64 builds.

## Assets

- [x86 uncompressed](https://github.com/stonechs/smolnes/releases/download/v1.6/smolnes-v1.6-clang-x86.exe): 9,216 B
- [x86 Crinkler 3.0b](https://github.com/stonechs/smolnes/releases/download/v1.6/smolnes-v1.6-clang-x86-crinkler.exe): 4,524 B
- [x64 uncompressed](https://github.com/stonechs/smolnes/releases/download/v1.6/smolnes-v1.6-clang-x64.exe): 10,752 B
- [x64 UPX 5.3.0 `--brute`](https://github.com/stonechs/smolnes/releases/download/v1.6/smolnes-v1.6-clang-x64-upx.exe): 8,192 B

SHA-256 hashes are listed in the [candidate asset manifest](../../dist/V1.6/candidate/release/SHA256SUMS.txt).

The x86 Crinkler build uses a native COFF object without LTO. The x64 build uses `-Oz -flto`; unwind tables are disabled for size. No runtime/gameplay test was performed for this packaging run. See the full [V1.6 release notes](../../dist/V1.6/candidate/RELEASE_NOTES.md) and [rebuild instructions](../../dist/V1.6/candidate/README.md).
