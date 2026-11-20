# smolnes V1.6 Clang candidate

V1.6 keeps the V1.5 emulator source and changes the Windows compiler and packer pipeline to Clang/LLVM 23.1.5.

## Four output files

| Architecture | Output | Size |
|---|---|---:|
| x86 | Clang + LLD uncompressed | 9,216 B |
| x86 | Clang native COFF + Crinkler 3.0b FAST | 4,524 B |
| x64 | Clang `-Oz -flto` + LLD uncompressed | 10,752 B |
| x64 | Clang `-Oz -flto` + UPX 5.3.0 `--brute` | 8,192 B |

All four files total **32,684 B**; the two compressed deliverables total **12,716 B**. Compared with V1.5's four files (33,020 B), the total is 336 B smaller. The x86 packed file is 1,360 B smaller than V1.5's UPack asset; the x64 UPX file is the same size.

## Build details

- x86 uses `-Oz`, function/data sections, and Crinkler with a native COFF object. Crinkler does not accept Clang LTO objects, so this target is not LTO.
- x86 uncompressed output uses LLD with imports, constants, and writable data merged into an RWX `.text` section to match the compact PE layout.
- x64 uses `-Oz -flto`, LLD, and UPX `--brute`.
- Both targets disable stack protection and unwind tables, omit the same V1.5 timer/error checks, and avoid CRT linking. No separate `memset.c` helper is used because the source/object has no `memset` reference.
- The source has not changed from V1.5. V1.5 release files were not modified.

Rebuild all four outputs with `build-v1.6-clang.cmd`. Build objects and maps are under `build-support/`; hashes are in `release/SHA256SUMS.txt`.

No additional runtime or gameplay test was run for this packaging trial.
