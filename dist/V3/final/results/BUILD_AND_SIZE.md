# V3 build and size record

- Source: exact V2-final-derived Stage 04 source.
- Compiler: WinLibs MinGW-w64 GCC 16.2.0, x86_64 UCRT.
- Link options: `-Os -flto -DNDEBUG -ffunction-sections -nostartfiles -nostdlib -static -mwindows -Wl,--gc-sections -s -Wl,--no-insert-timestamp -Wl,-e,mainCRTStartup`; explicit `user32`, `gdi32`, `kernel32`, `winmm`, and `gcc` libraries.
- Uncompressed EXE: 17,920 bytes.
- UPX: 5.3.0, `--best --lzma`.
- Packed EXE: 13,824 bytes (4,096 bytes / 22.86% smaller than the uncompressed image).
- UPX integrity test: PASS.
- `pig.NES` window, visible pixel grid, and clean-close smoke: PASS (center pixel `0x8C0000`, 13,440/13,440 nonblack samples).
- Missing-ROM prompt and nonzero exit smoke: PASS.

The corrected four-stage size table and archives are in the local `archive/V3-baselines/` directory. Earlier 15,360-byte experiments used a non-final source snapshot and are excluded from these results.
