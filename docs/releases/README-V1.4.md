# smolnes Windows-native V1.4 release candidate

The V1.x branch has a V1.4 dual-timer release candidate under [`dist/V1.4/candidate/`](../../dist/V1.4/candidate/). It contains only the extreme timer-size build: x64 uncompressed and UPX `--brute` (8,192 B), plus x86 uncompressed and UPack (6,012 B).

The nominal cadence is 16/16/18 ms. Timer setup failures are unchecked and timer shutdown cleanup is omitted. UPX integrity verification passed; runtime pacing and game compatibility have not been validated. UPack 0.39 states freeware for non-commercial use only.
