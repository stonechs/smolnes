# smolnes Windows-native V1.4 release candidate

- Replaces V1.3's frame wait with two periodic waitable timers, nominally producing 16/16/18 ms frame intervals.
- Ships only the extreme timer-size implementation; timer creation/start failure checks and timer shutdown cleanup are omitted.
- Provides x64 uncompressed (9,728 B) and UPX `--brute` (8,192 B) executables, plus x86 uncompressed (9,216 B) and UPack (6,012 B) executables.
- Preserves the source, build scripts, linker scripts, objects, maps, and per-file build information.
- UPX integrity verification passed. Runtime frame pacing and game compatibility have not been tested for V1.4.
