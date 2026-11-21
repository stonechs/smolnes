# smolnes Windows-native V3.3

V3.3 adds NES DMC (1-bit DPCM) playback to the V3.2 WinMM callback build. Sample reads use the existing mapper-aware memory reader; DPCM DAC changes are timestamped into the PCM stream. The DMC linear mix gain is 0.50. Pulse, triangle, and noise synthesis/mixing remain unchanged from V3.2. This version does not add integer/fixed-point mixing.

## ROM loading and controls

ROM loading and controls are unchanged from V3.2. No ROM is bundled.

## Four Windows outputs

| Output | Size |
| --- | ---: |
| `smolnes-v3.3-clang-x86-crinkler.exe` | 7,731 B |
| `smolnes-v3.3-clang-x86.exe` | 15,360 B |
| `smolnes-v3.3-clang-x64-upx.exe` | 12,800 B |
| `smolnes-v3.3-clang-x64.exe` | 18,432 B |

SHA-256 values and PE details are in `results/BUILD_AND_SIZE.md` and `release/SHA256SUMS.txt`.

## Audio notes and limits

- The DPCM output unit plays bits LSB-first and changes its 7-bit DAC by ±2 with saturation. Byte-boundary ordering was corrected against NESdev and Mesen2.
- DMC reads use mapper-aware ROM access. Headless Contra tracing confirmed the fetched bytes match the mapped ROM bank.
- DMC uses a linear gain of 0.50. This is the empirically selected listening setting, not the NES nonlinear TND mixer. The user confirmed the Contra drum is audible at this gain.
- DMC DMA is modeled as a four-cycle stall at the APU clock boundary; this is not cycle-perfect.
- No ROM is bundled. Keep runtime verification headless; no foreground GUI/audio device test is part of this package build.

## Rebuild

Run `build-v3.3.cmd` from this directory with Visual Studio 2022 Build Tools/Windows SDK, Clang/LLD 23.1.5, Crinkler 3.0b, and UPX 5.3.0. x86 uses Clang `-Oz`; x64 uses `-Oz -flto`. x86 is linked with Crinkler FAST; x64 is packed with UPX `--brute`. Run `build-support/tests/run-tests.cmd` for the headless DMC unit and PCM-mix checks.
