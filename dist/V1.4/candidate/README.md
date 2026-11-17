# smolnes Windows-native V1.4 — dual-timer release candidate

V1.4 is based on V1.3 and uses a 16 ms waitable timer for two frame waits and a 50 ms timer for the third. The nominal intervals are 16/16/18 ms. Each 50 ms event re-anchors the 16 ms timer.

## Release files

Only the extreme timer-size implementation is included as the V1.4 release build. It omits timer creation/start failure checks and timer shutdown cleanup.

| Architecture | Release asset | Packing | Size |
| --- | --- | --- | ---: |
| x64 | `release/smolnes-v1.4-dual-timer-x64.exe` | Uncompressed | 9,728 B |
| x64 | `release/smolnes-v1.4-dual-timer-x64-upx.exe` | UPX `--brute` | 8,192 B |
| x86 | `release/smolnes-v1.4-dual-timer-x86.exe` | Uncompressed | 9,216 B |
| x86 | `release/smolnes-v1.4-dual-timer-x86-upack.exe` | UPack 0.39 | 6,012 B |

## Rebuild and pack

`source/deobfuscated_win32.c`, the linker scripts, object files, maps, and build information are included. Build the uncompressed x86/x64 release pair with `build-v1.4.ps1`; pack copies of those files with `pack-v1.4-release.ps1`.

```powershell
.\build-v1.4.ps1 -GccPathX64 "C:\path\to\x64-gcc.exe" -GccPathX86 "C:\path\to\i686-gcc.exe"
.\pack-v1.4-release.ps1 -UpxPath "C:\path\to\upx.exe" -UpackPath "C:\path\to\Upack.exe"
```

The pack script preserves the uncompressed inputs, packs x64 with UPX `--brute`, packs x86 with UPack, runs UPX's `-t` check, and regenerates the release asset hashes. UPack's banner states that it is freeware for non-commercial use only; confirm that restriction is appropriate before distributing its output.

## Validation limits

Compile and linked-size records are included. Runtime frame pacing, game compatibility, and gameplay have not been tested for this V1.4 dual-timer candidate. Actual timer wakeups depend on Windows timer resolution and scheduling; 16/16/18 ms is the nominal schedule, not a timing guarantee.

`SHA256SUMS.txt` covers the executable and object files. UPX `-t` passed for the x64 packed file; UPack completed the x86 pack to 6,012 B. `release/release-assets-sha256.txt` covers the four release assets.
