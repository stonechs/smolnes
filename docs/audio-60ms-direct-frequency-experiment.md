# V3.1 Audio 60 ms / Direct Frequency Experiment

- Date: 2026-11-21
- Status: Four local listening candidates built; not released.

This is the corrected candidate: it restores the original six 10 ms audio blocks (60 ms queued), disables the approximately 1 ms frequency glide, and retains the earlier PolyBLEP pulse-edge correction. It can be compared with `dist/experiments/v3.1-audio-30ms-direct-frequency/` to isolate queue depth while keeping direct frequency changes.

All four builds succeeded with Clang/LLD 23.1.5. PE architectures were inspected; the UPX integrity test passed. SHA-256 values are alongside the outputs. No executable was launched, and ROM audio remains for the user's manual listening comparison.

The official V3.1 release files remain unchanged. Superseded frequency-glide experiment folders are kept separately and are not the corrected candidate linked in this report.
