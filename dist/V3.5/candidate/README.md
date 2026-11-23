# smolnes V3.5 candidate

This candidate adds 7z single-file LZMA1 ROM loading to the x86 build using the supplied static `LzmaCApi` library. The x64 build remains NES-only because the provided library is x86.

## Runtime behavior

- The release x86 build detects the 7z signature, passes the bytes read into the bounded input buffer to `sevenzip_extract_single_lzma1_fast`, and uses the decoder result. It does not run the archive-structure, input-boundary, unpacked-size, or iNES-header validation code.
- Define `SMOLNES_7Z_DEBUG` together with `SMOLNES_ENABLE_7Z_LZMA1` to compile the debug preflight checks. `build-v3.5-7z-debug.cmd` builds a debug validation executable and a matching build without the preflight, both with a test-only early exit after loading.
- The fast decoder expects trusted, complete archives. Release input is limited by the fixed buffer (1 MiB + 16 bytes); the ROM output buffer has the same capacity. Invalid or unsupported archives are not safe release inputs.
- Decoder workspace allocation uses `malloc/free` imported from `msvcrt.dll` through the small import definition in `build-support/msvcrt-malloc.def`; the x86 executable avoids static Universal CRT allocation code.
- Supported layout follows the supplied fast API: one file, one stream, one LZMA1 coder. Other methods are not supported. The supplied archive collection also contains three non-LZMA1 archives, which the regression script skips.

## Build and validation

- `build-v3.5.cmd` builds x86 LLD, x86 Crinkler, x64 LLD, and x64 UPX outputs.
- `test-v3.5-7z.ps1` builds debug-preflight and release-path test executables, extracts reference files with 7-Zip, and tests `D:\2026work\LzmaCApi\forTest` without opening the emulator window.
- Latest result: 116 LZMA1 archives passed in both modes; 3 archives using other methods were skipped. Report: `results/7z-validation-results.csv`.

## Output sizes

| Output | Bytes | V3.4 baseline | Change |
| --- | ---: | ---: | ---: |
| x86 LLD | 16,384 | 14,848 | +1,536 |
| x86 Crinkler | 8,516 | 7,594 | +922 |
| x64 LLD | 17,920 | 17,920 | 0 |
| x64 UPX | 12,288 | 12,288 | 0 |

## PE section comparison

| Output | Section | V3.4 virtual size → V3.5 | V3.4 raw size → V3.5 | Change |
| --- | --- | ---: | ---: | ---: |
| x86 LLD | `.text` | 1,244,472 → 2,293,648 | 14,336 → 14,848 | raw +512; virtual +1,049,176 |
| x86 LLD | `.lzma` | absent → 547 | 0 → 1,024 | new section |
| x64 LLD | `.text`, `.rdata`, `.data`, `.pdata` | unchanged | unchanged | 0 |
| x64 UPX | `UPX0`, `UPX1`, `UPX2` | unchanged | unchanged | 0 |

The x86 `.text` virtual increase mainly comes from the added 1 MiB archive input buffer; it does not add that buffer to the on-disk raw section. The new `.lzma` section contributes 1,024 raw bytes after PE file alignment. Crinkler output has zero entries in the PE section table, so it is compared by total file size only.

These are candidate builds, not a published release.
