# V1.4 dual-timer size comparison

The selected source is the counter-reset variant of the V1.3 dual-timer experiment. A static counter chooses the 16 ms timer for two frame waits, then the 50 ms timer for the third wait, and resets its phase. This removes the `% 3` selection from the earlier implementation.

## Checked build

| Architecture | Linked `.text$*` | EXE | V1.3 final EXE | Difference |
| --- | ---: | ---: | ---: | ---: |
| x64 | 7,504 B | 10,240 B | 10,240 B | 0 B |
| x86 | 7,080 B | 9,216 B | 9,728 B | -512 B |

## Optional extreme size build

This build defines `SMOLNES_OMIT_TIMER_FAILURE_CHECKS` and `SMOLNES_OMIT_TIMER_SHUTDOWN_CLEANUP`.

| Architecture | Linked `.text$*` | EXE | Checked EXE | Difference |
| --- | ---: | ---: | ---: | ---: |
| x64 | 7,328 B | 9,728 B | 10,240 B | -512 B |
| x86 | 6,912 B | 9,216 B | 9,216 B | 0 B |

The object files show `.text` as zero because LTO defers machine-code generation until link time. The linked code totals come from `.text$*` entries in the linker maps. The PE linker script merges those inputs into `.idata`; executable file sizes are rounded to 512-byte alignment.

A later `MsgWaitForMultipleObjects` variant waited on both timer handles but did not reduce linked code or EXE size. It remains in the dated archive and is not included in this candidate package.

No runtime timer/pacing or game compatibility tests were run for these builds. These measurements establish file and linked-code sizes only.
