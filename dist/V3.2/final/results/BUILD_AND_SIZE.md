# V3.2 build and size record

- Source: V3.1 final source with WinMM window-message audio refill. Synthesis and mix logic were retained; audio thread, event, critical section and unused audio statistics were removed.
- Toolchain: Clang/LLD 23.1.5; Crinkler 3.0b; UPX 5.3.0.
- x86 flags: `--target=i686-pc-windows-msvc -Oz -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections -DNDEBUG`; no LTO for Crinkler-compatible COFF. LLD uses `/NODEFAULTLIB`, `/OPT:REF`, `/OPT:ICF`, `/FIXED`, and merges `.rdata`, `.data`, and `.idata` into `.text`.
- x64 flags: `--target=x86_64-pc-windows-msvc -Oz -flto` with the same protection and section flags. LLD uses `/NODEFAULTLIB`, `/OPT:REF`, `/OPT:ICF`, and `/FIXED`.
- System libraries: `kernel32`, `user32`, `gdi32`, `comdlg32`, `winmm`, and `libcmt` (compiler helpers). x86 Crinkler: FAST, hash size 32, 100 tries, CALLS transform. x64 UPX: `--brute`; `upx -t` passed.

## Outputs

| Artifact | PE | Size | V3.1 delta |
|---|---|---:|---:|
| `smolnes-v3.2-clang-x86.exe` | PE32 x86, LLD | 14,336 B | -512 B |
| `smolnes-v3.2-clang-x86-crinkler.exe` | PE32 x86, Crinkler | 6,954 B | -322 B |
| `smolnes-v3.2-clang-x64.exe` | PE32+ x64, LLD | 17,408 B | -512 B |
| `smolnes-v3.2-clang-x64-upx.exe` | PE32+ x64, UPX | 11,776 B | -512 B |

PE machine fields and output sizes were checked. UPX integrity test passed. SHA-256 hashes are in `../release/SHA256SUMS.txt`. No GUI or ROM was launched; callback-driven audio refills, playback stability and gameplay remain runtime-unverified. The existing 9 Clang operator-precedence warnings in the emulator core remain unchanged.
