# smolnes Windows-native V1.4 candidate — dual timer

This candidate builds on V1.3 and replaces its frame wait with two periodic waitable timers. It is prepared for review on the V1.x branch; it is not yet a finalized release.

## Recommended candidate: checked build

The checked build keeps timer creation/start failure handling and shutdown cleanup. Its nominal frame interval pattern is 16, 16, 18 ms (50 ms per three frames).

| Architecture | File | Size |
| --- | --- | ---: |
| x64 | `checked/smolnes-v1.4-dual-timer-x64.exe` | 10,240 B |
| x86 | `checked/smolnes-v1.4-dual-timer-x86.exe` | 9,216 B |

## Optional size experiment

The `extreme/` builds omit timer setup failure checks and timer shutdown cleanup. They are included for comparison and are not the recommended release choice until reviewed.

| Architecture | File | Size |
| --- | --- | ---: |
| x64 | `extreme/smolnes-v1.4-dual-timer-extreme-x64.exe` | 9,728 B |
| x86 | `extreme/smolnes-v1.4-dual-timer-extreme-x86.exe` | 9,216 B |

## Source and rebuild

The selected counter-reset source is in `source/deobfuscated_win32.c`. The linker scripts, object files, linker maps, and per-binary build information are kept with the candidate. Rebuild the checked binaries with `build-v1.4.ps1`; use `build-v1.4-extreme.ps1` for the optional size experiment. The scripts accept compiler paths, for example:

```powershell
.\build-v1.4.ps1 -GccPathX64 "C:\path\to\x64-gcc.exe" -GccPathX86 "C:\path\to\i686-gcc.exe"
```

## Validation limits

The binaries were built and their linked code sizes were compared. Runtime frame pacing, ROM compatibility, and gameplay were not tested for this dual-timer candidate. Actual timer wakeups depend on Windows timer resolution and scheduling, so the nominal 16/16/18 ms schedule is not a timing guarantee.

`SHA256SUMS.txt` covers the executable and object files. `release-assets-options-sha256.txt` lists the four executable choices separately.
