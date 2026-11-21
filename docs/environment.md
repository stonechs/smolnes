# Verified build environment

Updated: 2026-11-20

## Clang/LLVM for Windows-native builds

- Bundle: Clang/LLVM 23.1.5, `x86_64-pc-windows-msvc` host package, from the official LLVM 23.1.5 release.
- Installation: user-wide LLVM installation; the install location is account-specific.
- PATH entry: the installation's `bin` directory.
- Commands: `clang.exe`, `clang-cl.exe`, and `lld-link.exe` resolve from the user PATH.
- Version was verified with `clang.exe --version` and `lld-link.exe --version`; the installed `clang.exe` hash matches the previously verified bundle.
- Current and historical V1.5–V1.7 build scripts use the PATH commands instead of a project-private compiler directory. The private `tools/clang/` bundle and archive were removed after the verified V1.7 rebuild.
- V1.7's four outputs were rebuilt successfully using the user-wide install on 2026-11-20. The rebuild produced different hashes for three outputs, so `dist/V1.7/candidate/release/SHA256SUMS.txt` was regenerated from the rebuilt files; the Crinkler x86 hash remained the same.
- Targets used: `i686-pc-windows-msvc` for x86 and `x86_64-pc-windows-msvc` for x64.
- Installation is user-wide, not machine-wide.
