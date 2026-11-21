# V3.1 build and size record

- Source: V3 final source plus V1.7-style dual waitable timers and ROM picker/simple-argument flow.
- Clang/LLD: LLVM 23.1.5; Crinkler 3.0b; UPX 5.3.0.
- x86 flags: `--target=i686-pc-windows-msvc -Oz -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections -DNDEBUG`; no LTO for Crinkler-compatible COFF objects. LLD uses `/NODEFAULTLIB`, `/OPT:REF`, `/OPT:ICF`, `/FIXED`, and merges `.rdata`, `.data`, and `.idata` into `.text`.
- x64 flags: `--target=x86_64-pc-windows-msvc -Oz -flto` with the same protection/section flags. LLD uses `/NODEFAULTLIB`, `/OPT:REF`, `/OPT:ICF`, and `/FIXED`.
- Explicit system imports: `kernel32`, `user32`, `gdi32`, `comdlg32`, and `winmm`. `libcmt.lib` is explicitly supplied to x86 and x64 for compiler-generated helpers (`__alldiv` on x86 and `_fltused` on x64); default libraries remain disabled.
- x86 Crinkler settings: `/COMPMODE:FAST /HASHSIZE:32 /HASHTRIES:100 /TRANSFORM:CALLS`.
- x64 UPX settings: `--brute`; `upx -t` passed.

## Outputs

| Artifact | PE | Bytes |
|---|---|---:|
| `smolnes-v3.1-clang-x86.exe` | PE32 x86, LLD | 14,848 |
| `smolnes-v3.1-clang-x86-crinkler.exe` | PE32 x86, Crinkler | 7,276 |
| `smolnes-v3.1-clang-x64.exe` | PE32+ x64, LLD | 17,920 |
| `smolnes-v3.1-clang-x64-upx.exe` | PE32+ x64, UPX | 12,288 |

All four output architectures were verified by inspecting PE headers. SHA-256 values are recorded in `../release/SHA256SUMS.txt`. No GUI or ROM runtime tests were performed. Nominal timer intervals are based on the programmed periods; real wake precision/FPS is unmeasured.
