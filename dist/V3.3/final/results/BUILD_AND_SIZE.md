# V3.3 build and size baseline

Date: 2026-11-21. Source: the V3.2 Windows-native callback version plus the DMC changes in this package. DMC gain is 0.50. The V3.2 release directory is unchanged.

## Toolchain and options

- Clang/LLD 23.1.5, Crinkler 3.0b, UPX 5.3.0.
- x86: Clang `-Oz`, no stack protector or unwind tables, function/data sections; LLD output and Crinkler FAST output from the same COFF objects. No LTO on the Crinkler-compatible x86 object set.
- x64: Clang `-Oz -flto`, no stack protector or unwind tables, function/data sections; linked with LLD and packed with UPX `--brute`.
- Nine existing operator-precedence warnings from the emulator core remain.

## Output sizes and PE checks

| Artifact | PE machine | Size | Change from V3.2 same artifact |
| --- | --- | ---: | ---: |
| `smolnes-v3.3-clang-x86.exe` | PE32 / 0x014C | 15,360 B | +1,024 B |
| `smolnes-v3.3-clang-x86-crinkler.exe` | PE32 / 0x014C | 7,731 B | +777 B |
| `smolnes-v3.3-clang-x64.exe` | PE32+ / 0x8664 | 18,432 B | +1,024 B |
| `smolnes-v3.3-clang-x64-upx.exe` | PE32+ / 0x8664 | 12,800 B | +1,024 B |

The LLD x86 `.text` raw size is 14,848 B, up 1,024 B from V3.2's 13,824 B. The LLD x64 `.text` raw size is 12,800 B, up 1,024 B from V3.2's 11,776 B. The x64 uncompressed section table is: `.text` VS 12,726 / raw 12,800; `.rdata` VS 3,292 / raw 3,584; `.data` VS 1,232,672 / raw 512; `.pdata` VS 24 / raw 512. x86 LLD merges `.rdata`, `.data`, and `.idata` into `.text`. UPX and Crinkler alter the section layout; compare their final file sizes, not the LLD section table.

| Artifact | SHA-256 |
| --- | --- |
| `smolnes-v3.3-clang-x86.exe` | `933A38EB37F82DF16955B7DAF9C0CECF2409741250479A39A84510C564478104` |
| `smolnes-v3.3-clang-x86-crinkler.exe` | `B25F7933E3A0CB8F3CAF0BD4EC7CEC2149A0B1D8928D20709481DFD68D7816E0` |
| `smolnes-v3.3-clang-x64.exe` | `B8FA061435A03F42983FCAE772C8BCA43514A17F3885E27047A3A02BE03544A1` |
| `smolnes-v3.3-clang-x64-upx.exe` | `C37483CC2F855218C2CFBF20CD30C5C1C0684F5BC9359EA1AC412EF0F7D1CDDE` |

## Headless validation

- `build-support/tests/run-tests.cmd`: DMC memory fetch, four-cycle DMA stall, IRQ/status, LSB-first DAC sequence, disable, and loop test passed.
- The PCM-mix test passed after the six-block/60 ms prefill and confirmed the 0.50 DMC scale reaches the output at the expected level.
- UPX 5.3.0 integrity test passed for the x64 packed output. PE machine fields match the requested architectures.
- The user previously listened to the x86 gain-0.50 candidate and confirmed the Contra drum is audible. No GUI, audio device, or foreground automation was launched as part of this build.
- A separate 1,200-frame headless Contra trace on the same DMC logic recorded 7,277 mapper-aware sample reads and zero byte mismatches against the fixed PRG bank. That trace predates the gain-only 0.50 adjustment; see `HEADLESS_CONTRA_TRACE_GAIN_008.md` for the full trace.

The four outputs and sources are the V3.3 baseline. No GitHub Release was created as part of publishing this version.
