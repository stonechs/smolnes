# V1.4 dual-timer candidate

V1.4 is a proposed update to the V1.x Windows-native line. It replaces V1.3's single deadline wait with two periodic waitable timers: a 16 ms timer for the first two frames and a 50 ms timer for the third. Each 50 ms event re-anchors the 16 ms timer. The nominal interval sequence is 16/16/18 ms.

The default candidate retains timer creation/start error handling and shutdown cleanup. Optional `extreme/` builds omit those paths to reduce size; they are clearly separated and are not the recommended release variant.

| Architecture | Checked | Extreme size experiment | V1.3 final |
| --- | ---: | ---: | ---: |
| x64 | 10,240 B | 9,728 B | 10,240 B |
| x86 | 9,216 B | 9,216 B | 9,728 B |

Build/runtime status: compile and static size records are included. Frame pacing, game compatibility, and gameplay were not runtime-tested for V1.4. This document describes a candidate, not a published release.
