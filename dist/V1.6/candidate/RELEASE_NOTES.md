# smolnes Windows-native V1.6

V1.6 is based on the V1.5 Windows-native source. The change for this release is the compiler and packing pipeline: both architectures use Clang/LLVM 23.1.5; x86 uses Crinkler, and x64 uses UPX.

## Release assets

- x86 uncompressed: 9,216 B
- x86 Crinkler 3.0b FAST: 4,524 B
- x64 uncompressed: 10,752 B
- x64 UPX 5.3.0 `--brute`: 8,192 B

The four assets total 32,684 B. The compressed x86/x64 assets total 12,716 B. The x86 target uses native COFF without LTO because Crinkler cannot consume the Clang LTO object. The x64 target uses `-Oz -flto`. Both targets omit unwind tables and stack protection. The x64 unwind setting can reduce exception stack-unwinding/debugger support.

## Compatibility and testing

The source and emulator behavior are unchanged from V1.5. No new automated runtime or gameplay test was performed for this packaging run. The x86 Crinkler asset is byte-identical to the previously launched Clang/Crinkler candidate. The x64 asset was built and packed but not launched in this run.

SHA-256 values for all four executables are in `release/SHA256SUMS.txt`.
