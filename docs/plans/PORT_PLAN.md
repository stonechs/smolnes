# smolnes SDL-free Windows port plan

## Goal

Create a Windows-native build from the archived SDL core, removing the SDL dependency while keeping the emulator core and the current keyboard controls. Keep the original SDL-based sources and build outputs intact. The working port is `src/deobfuscated_win32.c`; the normal build target will be `smolnes-win32`.

## Workflow and records

- Work in four ordered stages: ROM file interface, keyboard state, Windows window/message loop, then final GDI display.
- After each stage, compile the Windows executable and run `scripts/smoke-test.ps1` with a generated minimal NROM image. The smoke check verifies the process creates a window and exits cleanly after `WM_CLOSE`; stage-specific checks are recorded as well.
- Save an immutable stage snapshot under `archive/native-port/stage-NN-*`, including source, executable, smoke-test log, and SHA-256 manifest. Never overwrite an existing snapshot.
- Preserve existing untracked build and `dist` directories.

## Stages

| Stage | Work | Stage-specific smoke check | Status |
|---|---|---|---|
| 1. ROM file interface | Replace SDL RWops ROM loading with checked standard C file I/O; SDL remains for display and events. | Launch with a generated valid ROM; also verify a missing ROM reports an error and exits nonzero. | Complete; both checks passed |
| 2. Keyboard state | Replace SDL keyboard-state/scancode reads with a focus-aware Win32 keyboard-state adapter while the SDL window remains temporarily. | Confirm `GetAsyncKeyState` is imported, SDL keyboard polling is absent, and the emulator window opens and closes cleanly. | Complete; compile and smoke passed |
| 3. Window/message loop | Replace SDL window lifecycle and event polling with Win32 window creation and a native message loop; the window is temporarily blank until stage 4. | Launch and close cleanly; confirm the source and executable have no SDL dependency. | Complete; smoke passed without SDL2.dll |
| 4. GDI display | Complete the GDI framebuffer path: 256x224 visible BGR565 frame, 1024x840 presentation, repaint handling, and GUI error messages. | Launch and display a nonblack frame; close cleanly; verify the final executable imports no SDL DLL and runs without `SDL2.dll` beside it. | Complete; build, pixel, missing-ROM, and no-SDL checks passed |

## Acceptance criteria

- `src/deobfuscated_win32.c` is the readable, separately maintained Windows-native port; the unchanged SDL sources are in `archive/V3-baselines/root-sdl-original-2026-11-15/source/`.
- Each stage has a passing smoke-test record and a restorable archive snapshot.
- Final CMake target builds with the existing MinGW-w64 GCC toolchain and has no SDL link or runtime dependency.
- Report executable size, imported runtime DLLs, and any limitations that the smoke test cannot establish (for example, compatibility with commercial games).

## Progress log

- 2026-11-15: Plan created. Existing untracked `build-windows/`, `build-size-osgc/`, and `dist/` are preserved. Existing toolchain found at the WinLibs MinGW-w64 installation.
- 2026-11-15: SDL baseline smoke passed and was archived in `archive/native-port/00-baseline/`.
- 2026-11-15: Stage 1 built `deobfuscated_win32.exe`; valid-ROM window/close and missing-ROM nonzero-exit smoke checks passed. Immutable snapshot: `archive/native-port/stage-01-rom-file/`. Stage 2 is next.
- 2026-11-15: Stage 2 replaced SDL key-state polling with focus-aware Win32 `GetAsyncKeyState`; build and window smoke passed. Immutable snapshot: `archive/native-port/stage-02-keyboard/`.
- 2026-11-15: Stage 3 replaced SDL window/event APIs with a native Win32 window and message pump; build and no-SDL-runtime smoke passed. Immutable snapshot: `archive/native-port/stage-03-window-loop/`.
- 2026-11-15: Stage 4 completed the 16-bit BGR565 GDI path, GUI error dialogs, and size-oriented MinGW link flags. The synthetic NROM smoke ROM enables PPU output; pixel probe returned `0x636163`. Missing-ROM dialog exited with code 1. The release EXE passed the same pixel and missing-ROM checks from `dist/native/` without `SDL2.dll`. Immutable snapshot: `archive/native-port/stage-04-gdi-display/`.
- 2026-11-15: User reported unusual colors while running `pig.NES`. SDL's local format definitions confirm `BGR565` uses red mask `0x001F`, green `0x07E0`, and blue `0xF800`; the first GDI snapshot had red and blue reversed. Corrected the GDI masks and added a directional blue-pixel smoke fixture; expected `COLORREF 0x8C0000` passed. The corrected release also launched `roms/commercial/pig.NES` and returned a nonblack center pixel. Immutable correction snapshot: `archive/native-port/hotfix-01-bgr565-color-order/`.
- 2026-11-15: Final corrected `smolnes-win32.exe` size: 28,160 bytes. PE import inspection found no SDL DLL; imported Windows components are GDI32, USER32, KERNEL32, and UCRT API-set DLLs. Only startup/frame output was checked with `pig.NES`; gameplay and broader ROM compatibility remain untested.

## V1 freeze and GitHub upload

- Local V1 snapshot: `archive/V1-release/` (source, CMake integration, `build-v1.ps1`, README, 28,160-byte executable, smoke record, SHA-256 manifest).
- GitHub fork: `https://github.com/stonechs/smolnes`.
- Uploaded branch: `v1-windows-native`; commit `e8b35e06d7b55754095211d1aa32243797735ee7`.
- Final version tag: `v1`.
- GitHub Release: `https://github.com/stonechs/smolnes/releases/tag/v1`; asset `smolnes-v1.exe` (28,160 bytes, SHA-256 `FB26AA5ED06306400792743912D93EB5A9611B1BB5BBD7D4EA8C8B2A9BF75200`).
- Uploaded files: `deobfuscated_win32.c`, `build-v1.ps1`, `README-V1.md`, updated `CMakeLists.txt`, `dist/V1/smolnes-v1.exe`, and `V1-SHA256SUMS.txt`.

## V1.1 frame pacing

- User reported that V1 felt slower than the SDL build. The V1 loop called `Sleep(16)` after rendering every frame, adding a fixed wait on top of emulation and GDI drawing time.
- V1.1 replaces that delay with a one-shot waitable timer scheduled against `QueryPerformanceCounter` deadlines at 60 Hz. High-resolution waitable timers are used when available, with a standard timer fallback. Expired deadlines are skipped so missed frames do not build up; `MsgWaitForMultipleObjects` keeps window messages flowing during the wait.
- V1 remains frozen. `build-v1.ps1` now explicitly builds from `archive/V1-release/source/`; `build-v1.1.ps1` builds the updated source to `dist/V1.1/`.
- Verification: `pig.NES` opened and closed cleanly; GDI center-pixel probe returned the expected `0x8C0000`; missing-ROM handling exited with code 1. Frame diagnostics measured original V1 at 27–31 FPS and V1.1 at 59–60 FPS after warm-up. An 8 ms/frame overrun stress build measured 40–44 FPS, with no extra full-period wait. Four-second process sampling showed 28.4% of one core for V1 and 58.4% for V1.1 (about 7.1% and 14.6% normalized across this machine's 4 logical processors). Immutable candidate snapshot: `archive/V1.1-frame-pacing/`.
