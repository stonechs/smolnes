# V3.4 build and size record

Date: 2026-11-21

## Build configuration

- Compiler: installed Clang/LLD, Windows MSVC target.
- x86 Clang flags: `-Oz`, no stack protector or unwind tables, function/data sections.
- x64 Clang flags: same size flags plus `-flto`.
- x86 compressed output: Crinkler 3.0b FAST mode with the V3.3 link options.
- x64 compressed output: UPX 5.3.0 `--brute`; `upx -t` passed.
- Build command: `build-v3.4.cmd`.

The build completed without errors. The compiler emitted nine existing precedence warnings from `deobfuscated_win32.c`; LLD also reported that `.idata` was already merged into `.text`.

## Outputs

| Output | Architecture | Size | V3.3 size | Difference |
| --- | --- | ---: | ---: | ---: |
| `smolnes-v3.4-clang-x86.exe` | x86 PE32 | 14,848 B | 15,360 B | -512 B |
| `smolnes-v3.4-clang-x86-crinkler.exe` | x86 PE32 | 7,594 B | 7,731 B | -137 B |
| `smolnes-v3.4-clang-x64.exe` | x64 PE32+ | 17,920 B | 18,432 B | -512 B |
| `smolnes-v3.4-clang-x64-upx.exe` | x64 PE32+ | 12,288 B | 12,800 B | -512 B |

The V3.4 changes combine integer audio, a 60fps audio clock and a 50ms queue, so these whole-file size differences are not an isolated measurement of integer arithmetic alone.

## Checksums

See `release/SHA256SUMS.txt` for SHA-256 values. All four files were inspected for the expected PE machine and optional-header format. No GUI, ROM, or audio-device run was performed on these exact V3.4 files.
