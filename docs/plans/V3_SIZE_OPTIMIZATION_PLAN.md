# V3 executable size reduction plan

Baseline: `archive/V3-baselines/v3-size-000-baseline/` — 32,256-byte V3 `-Oz -flto` EXE, source, object files, build script and smoke logs.

Each stage gets an isolated source/build folder and records EXE bytes, PE section sizes, imports, object sizes, SHA256, smoke results, and a source+script+binary snapshot. A stage is retained as the new best only if it builds, passes smoke checks, and reduces size without breaking required behavior.

1. **Win32 ROM file I/O:** replace `fopen/fread/ferror/fclose` with `CreateFileA/ReadFile/CloseHandle`; keep the current CRT command-line entry to isolate file-I/O impact. Record size delta and smoke ROM/missing ROM.
2. **Argument handling and custom entry:** stop using `__argc/__argv`; first test the `WinMain` command-line route, then custom PE entry with `-nostartfiles`. Record each sub-build separately. Test quoted paths, ROM load, clean exit and audio startup.
3. **Selective no-CRT link:** try `-nostdlib` and explicitly link only required Windows libraries and unavoidable compiler helpers. Resolve undefined symbols deliberately; reject if runtime behavior fails or the size is not lower.
4. **PE layout/resources:** inspect resources and section padding; test only safe removals/merges. Do not lower PE file alignment below the supported value or disable ASLR just to save a few bytes. Record measured section and whole-file delta.

At every stage compare against the best prior valid build, not just the immediately previous experiment.

## Execution results (2026-11-15)

| Build | EXE size | Change vs previous |
|---|---:|---:|
| V3 `-Oz -flto` baseline | 32,256 bytes | — |
| Stage 01 Win32 ROM file API | 32,256 bytes | 0 |
| Stage 02a WinMain argument parsing | 32,768 bytes | +512 |
| Stage 02b custom entry + `-nostartfiles` | 22,016 bytes | -10,752 |
| Stage 03 custom entry + `-nostdlib` | 15,360 bytes | -6,656 |
| Stage 04 PE layout audit | 15,360 bytes | 0 |

Stage 03 is the current best: 16,896 bytes smaller than the original V3 baseline (52.38%). Stages 01–04 have source/build/object/binary/result snapshots under `archive/V3-baselines/`. Stage 03 passed the ROM image/window smoke and missing-ROM error smoke. This is a smoke result, not a broad game-compatibility or audio-quality regression pass.

## Baseline correction (2026-11-15)

The initial V3 size experiment used `dist/V2/smolnes-v2-package/source`, which is not the V2-final source snapshot. Its APU and audio source differ from `archive/V2-final`; therefore the 32,256-byte and 15,360-byte builds are not valid V2-final size/audio comparisons. A corrected four-way flag matrix using the exact V2-final sources is archived at `archive/V3-baselines/v3-flag-matrix-v2-final-source/`. The `-Os` build matches the known-good V2 executable size (33,792 bytes). Audio is awaiting the user's same-scene listening comparison for the LTO variants.

## Four stages rebuilt from V2-final source (2026-11-15)

| Stage | Executable | Size | Delta from prior stage |
|---|---|---:|---:|
| 01 Win32 ROM file API | `corrected-stage-01-rom-api/smolnes-v3.exe` | 33,280 | baseline |
| 02 custom entry, `-nostartfiles` | `corrected-stage-02-custom-entry/smolnes-v3.exe` | 23,040 | -10,240 |
| 03 `-nostdlib` | `corrected-stage-03-nostdlib/smolnes-v3.exe` | 17,920 | -5,120 |
| 04 PE layout audit | `corrected-stage-04-pe-audit/smolnes-v3.exe` | 17,920 | 0 |

Stage 01 is 512 bytes smaller than V2 (33,792 bytes) due to the selected `-Os -flto` baseline; the file-API change itself did not further reduce the file. All four stages passed the ROM image/window/close and missing-ROM smoke checks. Each stage snapshot, including its source, `.o` files, script, binary, link map, and logs, is under `archive/V3-baselines/v3-final-source-stage-*`.

The user-confirmed normal audio applies to the full-CRT optimization flag matrix. The custom-entry and `-nostdlib` stage builds still need listening validation before release.

## Corrected rerun status (2026-11-15)

The earlier size table above used a non-final source snapshot and is historical only. Its 15,360-byte result is not a valid V2-final-derived build. The four stages have now been rebuilt from `archive/V2-final` with the common `-Os -flto` baseline; use the corrected table in `archive/V3-final/results/V3_SIZE_OPTIMIZATION_RESULTS.md` as the current result. Corrected build stages are archived under `archive/V3-baselines/v3-final-source-stage-01-rom-api` through `...stage-04-pe-audit`.
