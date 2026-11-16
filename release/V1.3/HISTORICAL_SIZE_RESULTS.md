# V1.3 size results

Base source: `archive/V1.2-scanline/stage-03-scanline-compose/source/deobfuscated_win32.c` (SHA-256 `CC9C3F0D34FAA91F698C33424036146B3B2EE261D71D8CA6243E49A9E46AACF4`). Baseline release executable: 29,184 bytes. Compiler: WinLibs MinGW-w64 GCC 16.2.0 x86_64 UCRT.

| Stage | Change | EXE | `.o` | Change from previous |
|---|---|---:|---:|---:|
| V1.2 baseline | Original source and original V1.2 flags | 29,184 B | 22,317 B | — |
| Optimized flags | `-Os -flto`, same source | 29,184 B | 62,580 B | 0 B |
| 01 | Replace ROM stdio with `CreateFileA` / `ReadFile` / `CloseHandle` | 29,184 B | 62,800 B | 0 B |
| 02 | Custom entry point and `-nostartfiles` | 18,432 B | 64,854 B | -10,752 B |
| 03/04 | `-nostdlib`, explicit Windows imports and PE audit | 13,312 B | 64,790 / 64,806 B | -5,120 B |
| Smallest experiment | `-Oz`, no unwind metadata | 12,288 B | 65,190 B | -1,024 B |

The 13,312-byte candidate is 15,872 bytes (54.4%) smaller than V1.2. The smallest measured experiment is 16,896 bytes (57.9%) smaller. The 12,288-byte build saves one more 1,024-byte file-alignment block by removing `.pdata` and `.xdata`; this trades away some Windows exception stack-walking/crash-diagnostic support and is therefore kept as experimental.

The object files produced with LTO contain compiler intermediate representation; their larger file size is not the final native `.text` size.

## `.text` function distribution

The sizes below come from the linker maps after LTO, so inlined helpers are charged to the surviving output function. The compiler folded the emulator entry, ROM setup, main emulation loop, and shutdown path into `mainCRTStartup`.

| Linked function | Unwind retained | Smallest build | Share of `.text` (retained / smallest) |
|---|---:|---:|---:|
| `mainCRTStartup` (includes inlined `run_emulator` and parser) | 5,568 B | 5,456 B | 69.6% / 69.3% |
| `mem` | 1,712 B | 1,696 B | 21.4% / 21.5% |
| `window_proc` | 288 B | 288 B | 3.6% / 3.7% |
| `get_nametable_byte` | 96 B | 96 B | 1.2% / 1.2% |
| `pump_native_messages` | 96 B | 96 B | 1.2% / 1.2% |
| `advance_frame_deadline` | 80 B | 80 B | 1.0% / 1.0% |
| `get_chr_byte` | 64 B | 64 B | 0.8% / 0.8% |
| `read_pc` | 64 B | 64 B | 0.8% / 0.8% |
| `set_nz.isra.0` | 32 B | 32 B | 0.4% / 0.4% |
| **Total `.text` bytes** | **8,000 B** | **7,872 B** | **100%** |

Both candidates reserve the same 8,192-byte on-disk `.text` allocation after PE 512-byte file alignment. The smallest build's 128-byte code reduction does not itself reduce the `.text` file allocation. Its 1,024-byte EXE saving comes from removing the separately aligned `.pdata` and `.xdata` sections.

## Other PE sections

| Section | Safe virtual bytes | Safe file bytes | Smallest virtual bytes | Smallest file bytes | Contents |
|---|---:|---:|---:|---:|---|
| PE headers | 1,024 | 1,024 | 1,024 | 1,024 | DOS/PE headers and section table |
| `.text` | 8,000 | 8,192 | 7,872 | 8,192 | Executable machine code |
| `.data` | 16 | 512 | 16 | 512 | Initialized writable globals |
| `.rdata` | 736 | 1,024 | 736 | 1,024 | Read-only constants and strings |
| `.pdata` | 108 | 512 | absent | absent | x64 function unwind index |
| `.xdata` | 88 | 512 | absent | absent | x64 unwind descriptions |
| `.bss` | 1,232,256 | 0 | 1,232,256 | 0 | Zero-initialized emulator state, including ROM buffer/framebuffer |
| `.idata` | 1,436 | 1,536 | 1,436 | 1,536 | DLL import metadata |
| **EXE total** | — | **13,312** | — | **12,288** | File sizes include PE alignment |

The `.bss` reservation is about 1.18 MiB in memory but contributes no bytes to the EXE because Windows zero-fills it when loading. The `.idata` raw file size is rounded from 1,436 to 1,536 bytes; the remaining 100 bytes are section-alignment padding.

## `.idata` contents

The V1.3 import table has 33 named APIs across three DLLs: `USER32.dll` 20, `KERNEL32.dll` 12, and `GDI32.dll` 1. The linker map and PE import table agree on these totals.

| Import-table component | Bytes | Share of `.idata` virtual size | Meaning |
|---|---:|---:|---|
| DLL descriptors | 60 | 4.2% | Three 20-byte DLL descriptors |
| Null descriptor | 20 | 1.4% | Terminates the descriptor list |
| ILT (lookup table) | 288 | 20.1% | 33 64-bit API-name references plus three DLL terminators |
| IAT (address table) | 288 | 20.1% | Matching slots that Windows fills with resolved API addresses |
| API hint/name records | 608 | 42.3% | Import-by-name records (hint and NUL-terminated API names) |
| DLL-name records | 172 | 12.0% | DLL-name references and the three DLL name strings |
| **`.idata` virtual total** | **1,436** | **100%** | Plus 100 bytes of file-alignment padding on disk |

Thus the API names are required loader metadata, not debug symbols. Their 608 bytes are the largest `.idata` component; removing them would require changing how the executable resolves APIs.

## Sub-512-byte alignment experiment

GNU `ld` accepted `--file-alignment=0x100 --section-alignment=0x100` and produced an 11,264-byte file, but Windows rejected it at process creation as an invalid application. The PE low-alignment rule requires section data's file offset to match its RVA. This build's default layout places the large zero-fill `.bss` before `.idata`, leaving `.idata` at RVA `0x12F400` but file offset `0x2600`; the low-alignment image therefore does not satisfy that layout rule. The rejected image is preserved under `size-experiments/subpage-align-100/` for diagnosis and is not a V1.3 deliverable.

GNU `ld` exposes `--file-alignment` and `--section-alignment`. The PE specification's usual file-alignment range starts at 512 bytes; below-page section alignment requires matching file alignment and stricter section placement. Reaching a runnable sub-512-byte layout would need a custom linker layout (for example, placing the zero-fill `.bss` after file-backed sections) and another Windows loader/game smoke pass.

### Custom linker-layout follow-up

The default GCC 16.2.0 PE linker script was copied and changed to place `.bss` after `.idata`, then linked with matching `0x100` file and section alignment. This removed the `.bss`-before-`.idata` offset mismatch: all file-backed sections had `PointerToRawData == RVA`. The 11,264-byte output still failed Windows process creation with `ERROR_BAD_EXE_FORMAT`. A reordered `0x200`/`0x200` control build also failed to launch. Patching the empty `.bss` raw pointer to its RVA did not fix the 0x100 build.

Adding a zero-byte anchor inside `.bss` made Windows launch the program and the `pig.NES` pixel smoke pass, but caused the entire 1.2 MiB zero-fill area to become file-backed; that build was 1,243,648 bytes. This confirms that section reordering alone does not yield a smaller runnable EXE with this toolchain and Windows loader. Keep the 12,288-byte candidate as the smallest runnable build found so far; no custom low-alignment output replaces it.

## Smoke checks

All six stage outputs and both 12,288-byte experiments passed the existing `pig.NES` window/pixel/clean-close check (`center_pixel=0x8C0000`) and the missing-ROM dialog/nonzero-exit check. These are smoke checks, not a broad compatibility test. Audio is not part of V1.2/V1.3.
