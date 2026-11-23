# smolnes V3.5 candidate

This candidate adds 7z single-file LZMA1 ROM loading to the x86 build by compiling `src/lzmacapi/SRC/sevenzip_single_fast.c` with Clang and assembling `src/lzmacapi/SRC/lzma_static_x86.asm` with NASM. The repository-root `src/` is the source of truth; the candidate's `source/` and `vendor/lzmacapi/` directories mirror the V3.5 source snapshot and its attribution. The x64 build remains NES-only because this decoder is x86-only.

## Runtime behavior

- The release x86 build detects the 7z signature, passes the bytes read into the bounded input buffer to `sevenzip_extract_single_lzma1_fast`, and uses the decoder result. It does not run the archive-structure, input-boundary, unpacked-size, or iNES-header validation code.
- Define `SMOLNES_7Z_DEBUG` together with `SMOLNES_ENABLE_7Z_LZMA1` to compile the debug preflight checks. `build-v3.5-7z-debug.cmd` builds a debug validation executable and a matching build without the preflight, both with a test-only early exit after loading.
- The fast decoder expects trusted, complete archives. Release input is limited by the fixed buffer (1 MiB + 16 bytes); the ROM output buffer has the same capacity. Invalid or unsupported archives are not safe release inputs.
- Decoder workspace allocation uses `malloc/free` imported from `msvcrt.dll` through the small import definition in `build-support/msvcrt-malloc.def`; the x86 executable avoids static Universal CRT allocation code.
- Supported layout follows the supplied fast API: one file, one stream, one LZMA1 coder. Other methods are not supported. The supplied archive collection also contains three non-LZMA1 archives, which the regression script skips.

## Build and validation

- `build-v3.5.cmd` builds x86 LLD, x86 Crinkler, x64 LLD, and x64 UPX outputs from repository-root `src/`, compiling the decoder C and assembly sources directly.
- `test-v3.5-7z.ps1` builds debug-preflight and release-path test executables from root `src/`, extracts reference files with 7-Zip, and tests `D:\2026work\LzmaCApi\forTest` without opening the emulator window.
- Headless regression on the current direct-source build: 116 LZMA1 archives passed in both debug and release paths, 3 unsupported methods were skipped, and there were 0 failures. The test loads/decodes each archive then exits before emulation; GUI, gameplay, and audio-device testing remain unperformed. Report: `results/7z-validation-results.csv`.

## Output sizes

| Output | Bytes | V3.4 baseline | Change |
| --- | ---: | ---: | ---: |
| x86 LLD | 16,384 | 14,848 | +1,536 |
| x86 Crinkler | 8,431 | 7,594 | +837 |
| x64 LLD | 17,920 | 17,920 | 0 |
| x64 UPX | 12,288 | 12,288 | 0 |

## PE section comparison

| Output | Section | V3.4 virtual size → V3.5 | V3.4 raw size → V3.5 | Change |
| --- | --- | ---: | ---: | ---: |
| x86 LLD | `.text` | 1,244,472 → 2,293,568 | 14,336 → 14,848 | raw +512; virtual +1,049,096 |
| x86 LLD | `.lzma` | absent → 547 | 0 → 1,024 | new section |
| x64 LLD | `.text`, `.rdata`, `.data`, `.pdata` | unchanged | unchanged | 0 |
| x64 UPX | `UPX0`, `UPX1`, `UPX2` | unchanged | unchanged | 0 |

The x86 `.text` virtual increase mainly comes from the added 1 MiB archive input buffer; it does not add that buffer to the on-disk raw section. The new `.lzma` section contributes 1,024 raw bytes after PE file alignment. Crinkler output has zero entries in the PE section table, so it is compared by total file size only.

## Direct-source build size accounting

The official x86 build now compiles the vendored C and assembly directly. The previous static-library link experiment explains the earlier `-87 B` figure: that is the direct-source map reduction versus the `.lib` link, not the increase versus V3.4.

| Metric | V3.4 baseline | V3.5 direct source | Change |
| --- | ---: | ---: | ---: |
| x86 LLD `.text` virtual / raw | 1,244,472 / 14,336 B | 2,293,568 / 14,848 B | +1,049,096 / +512 B |
| x86 LLD `.lzma` virtual / raw | absent | 547 / 1,024 B | +547 / +1,024 B |
| x86 LLD EXE | 14,848 B | 16,384 B | +1,536 B |
| x86 Crinkler EXE | 7,594 B | 8,431 B | +837 B |

Map function extents show `_run_emulator` grew by 152 B (5,453 → 5,605 B); the new C decoder entry and `read_number_fast` occupy about 221 B and 87 B, respectively. The `.lzma` assembly contributes 547 B. Thus the decoder C+assembly occupies 855 B in this integrated build. The earlier 871 B number was from the standalone LzmaCApi map (245 + 79 + 547 B), with different C function boundaries/compiled sizes. These function extents and whole-program section deltas are separate accounting views and should not be added together.

The map's initial `.text` contribution is 0x2C73 B in V3.4 and 0x2E28 B in this build, a net increase of 437 B. This is the linked section contribution, not a sum of only newly added functions.

The PE file grows by 1,536 B because `.text` raw data increases by one 512-byte file-alignment unit and the new `.lzma` section occupies 1,024 raw bytes. The `.text` virtual increase also includes the 1 MiB archive buffer. Crinkler has no standard PE section table, so its output is compared by total file size.

These are candidate builds, not a published release.
