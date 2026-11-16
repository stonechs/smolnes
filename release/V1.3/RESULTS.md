# V1.3 native-caption property-only experiment

This candidate uses the normal Windows caption as its drag area. The only window-style change is `WS_CAPTION | WS_SYSMENU`; it has no custom mouse-drag handlers. `DefWindowProcA` remains so Windows can process non-client caption behavior. `AdjustWindowRect` was removed; the requested outer window size is passed directly to `CreateWindowExA`, so the client area is slightly smaller than 1024x840.

| Architecture | Native-caption build | No-drag one-section baseline | Manual-drag build |
|---|---:|---:|---:|
| x64 | **10,240 B** | 10,240 B | 10,752 B |
| x86, `.reloc` disabled | **9,728 B** | 9,728 B | 10,240 B |

The PE file sizes are unchanged from the no-drag baseline and 512 bytes below the manual-drag build. The maps contain `DefWindowProcA`; they do not contain `AdjustWindowRect`, `SetWindowPos`, `GetCursorPos`, `GetWindowRect`, `SetCapture`, or `ReleaseCapture`.

Both x64 and x86 compiled successfully with the same V1.3 one-section build settings. The x86 image remains fixed-base with `.reloc` disabled. Direct UI verification of dragging was interrupted before completion, so the drag behavior still needs confirmation in the running window.

Artifacts: source, `build-both.ps1`, linker scripts, objects, maps, build metadata, and executables are stored in this directory.

## Final rebuild and packed deliverables

Rebuilt both architectures from the snapshot source with `build-both.ps1`.
The uncompressed executable hashes match the prior recorded build. UPack
compressed the x86 copy. UPX initially rejected the 10 KB x64 custom-layout
image with its default search; the single `--ultra-brute` option found a valid
encoding, and `upx -t` passed.

| Artifact | Size | SHA-256 |
| --- | ---: | --- |
| `smolnes-v1.3-native-caption-x64.exe` | 10,240 B | `EF0A266034EBCAA63615A4746F34BED7ED814B477D780939BC485EF6B3C2D9EE` |
| `smolnes-v1.3-native-caption-x64-upx.exe` | 8,192 B | `74E278CEB79944086517DA28D3642029248DDB080B2D40CE4497CB44E7B21B4C` |
| `smolnes-v1.3-native-caption-x86.exe` | 9,728 B | `1FEEFDCCA6339DED3AD7673A73A9BEED5CA0353935D833CE8BB4F512E7105019` |
| `smolnes-v1.3-native-caption-x86-upack.exe` | 6,204 B | `F4CF99F3B92B16BEE228D8445726349EE19A4ECA14169FC6D0787E5CA1567046` |

Packed executable files are copies; the normal x86/x64 binaries remain intact.
