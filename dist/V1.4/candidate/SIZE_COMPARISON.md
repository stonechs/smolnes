# V1.4 dual-timer size and packing comparison

V1.4 uses the counter-reset dual-timer source. A static counter selects the 16 ms timer for two frame waits, then the 50 ms timer for the third wait, and resets its phase. This removes `% 3` from timer selection.

The extreme timer-size build is the only V1.4 release variant. It defines `SMOLNES_OMIT_TIMER_FAILURE_CHECKS` and `SMOLNES_OMIT_TIMER_SHUTDOWN_CLEANUP`; timer setup failures are unchecked and exit cleanup is omitted.

| Architecture | Linked `.text$*` | Uncompressed | Packed release | Checked dual-timer comparator | V1.3 final EXE |
| --- | ---: | ---: | ---: | ---: | ---: |
| x64 | 7,328 B | 9,728 B | **8,192 B** (UPX `--brute`) | 10,240 B | 10,240 B |
| x86 | 6,912 B | 9,216 B | **6,012 B** (UPack 0.39) | 9,216 B | 9,728 B |

The checked comparison executables are not included in the V1.4 release package; their records remain in the dated experiment archive. With LTO, object `.text` sizes are zero because machine-code generation occurs at link time. Linked code totals come from `.text$*` entries in the maps. The custom linker layout merges those inputs into `.idata`; on-disk executable sizes are rounded to 512-byte alignment.

A later `MsgWaitForMultipleObjects` variant offered no size reduction and is not selected. No runtime timer/pacing or game-compatibility tests were run for V1.4. UPX integrity verification passed with `upx -t`; UPack completed successfully, but UPack 0.39 does not provide an equivalent integrity check in its displayed options.
