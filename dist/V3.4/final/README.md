# smolnes Windows-native V3.4

V3.4 is prepared locally for review before publication. It changes the audio synthesis and mixing path to integer/fixed-point arithmetic and uses a 50ms WinMM queue. V3.3 remains unchanged; no GitHub release or push has been made.

## Audio changes

- Pulse, triangle, and noise synthesis use integer phase, integer frequency steps, Q15 gain and integer mixing/clipping.
- DMC DAC mixing and sample-cycle accumulation use integer arithmetic. The 0.50 linear DMC gain is approximated as 256 PCM units per DAC step.
- Audio clock uses 1,786,840 cycles/s, aligned to the emulator's nominal 60fps pacing. DMC events stay on the simulation cycle timeline; the queued audio supplies playback latency.
- Queue contains five 10ms blocks (480 stereo frames each at 48kHz), totaling 50ms.
- Existing WinMM window callback, mapper-aware DMC reads, controls, ROM loading, and dual-timer frame pacing remain.

The integer path is approximate, not bit-exact to the floating-point V3.3 path. No nonlinear TND mixer or cycle-perfect DMC DMA is included.

## Windows outputs

| Output | Size |
| --- | ---: |
| `smolnes-v3.4-clang-x86.exe` | 14,848 B |
| `smolnes-v3.4-clang-x86-crinkler.exe` | 7,594 B |
| `smolnes-v3.4-clang-x64.exe` | 17,920 B |
| `smolnes-v3.4-clang-x64-upx.exe` | 12,288 B |

SHA-256 values, PE architecture, and build notes are in `results/BUILD_AND_SIZE.md` and `release/SHA256SUMS.txt`.

## Verification status

All four outputs built. PE machine/optional-header formats were checked; UPX integrity test passed. Runtime ROM playback and listening of this exact V3.4 build have not been tested. It is staged for manual review; no ROM is bundled.

## Rebuild

Run `build-v3.4.cmd` from this directory with the installed Clang/LLD, Visual Studio 2022 Build Tools/Windows SDK, Crinkler 3.0b, and UPX. x86 uses Clang `-Oz`; x64 uses `-Oz -flto`. x86 Crinkler uses FAST mode; x64 is packed with UPX `--brute`.
