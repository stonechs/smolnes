# V3.1 Audio 30 ms / Frequency Glide Experiment

- Date: 2026-11-21
- Status: Four local listening candidates built; not released.

This A/B candidate keeps the queue at three 10 ms blocks (30 ms), restores the prior per-sample frequency glide (`frequency += (target - frequency) * 0.02`, about a 1 ms time constant), and retains the PolyBLEP pulse-edge correction. Other audio and emulator logic is unchanged from the preceding 30 ms candidate.

Clang/LLD 23.1.5 built all four outputs. PE architectures were inspected; the UPX integrity test passed. Outputs and SHA-256 values are in `dist/experiments/v3.1-audio-30ms-frequency-glide/release/`. No GUI was launched and actual ROM audio has not been listened to by the agent.

The official `dist/V3.1/final/` binaries remain byte-for-byte unchanged. No commit was made because the starting worktree has extensive pre-existing staged, modified and untracked content, so task changes cannot be safely isolated for commit.
