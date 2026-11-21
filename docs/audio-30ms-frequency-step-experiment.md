# V3.1 Audio Queue / Frequency Experiment

- Date: 2026-11-21
- Status: Four local listening candidates built; not released.

## Changes

- Audio queue depth: 6 blocks to 3 blocks. At 480 frames per block and 48 kHz, queued audio is nominally 30 ms instead of 60 ms.
- Oscillator frequency: direct assignment of the current target; removed the per-sample 0.02 exponential glide (~1 ms time constant).
- Retained the prior PolyBLEP pulse-edge correction. Thus these candidates combine that earlier experiment with the two newly requested changes.
- No APU envelope logic, gain smoothing, noise, pulse duty, mixing, or frame pacing changes.

## Build results

Built with Clang/LLD 23.1.5 from the exact V3.1 `deobfuscated_win32.c` and `apu.c` sources plus the experiment's `audio_win32.c`. The four outputs are in `dist/experiments/v3.1-audio-30ms-direct-frequency/release/`:

| Artifact | PE verification | Bytes |
|---|---|---:|
| x86 LLD | PE32 x86 | 15,360 |
| x86 Crinkler | PE32 x86 | 7,380 |
| x64 LLD | PE32+ x64 | 17,920 |
| x64 UPX | PE32+ x64 | 12,288 |

UPX integrity test passed. SHA-256 values are recorded alongside the binaries. No executable was launched; actual ROM audio and perceived improvement remain for the user to compare.

## Preservation and workspace

- Formal V3.1 release files were not overwritten.
- The primary worktree already contained extensive staged, modified, and untracked content before this experiment. No commit was made because the modified `src/`, `scripts/`, and `docs/` trees are untracked as a whole and cannot be safely staged as isolated diffs.
