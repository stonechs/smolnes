# smolnes

Compact NES emulator in C with Windows-native V1.X and V3 development lines. The supported mapper set is 0, 1, 2, 3, 4, and 7.

## Changes from upstream

The Windows-native versions are based on [binji/smolnes](https://github.com/binji/smolnes) and keep its compact C emulator core while replacing its SDL host layer and extending rendering, audio, timing, and ROM loading:

- **No SDL dependency:** Win32 handles the window, messages, keyboard input, and ROM picker; GDI presents the framebuffer, and WinMM outputs audio. The Windows builds do not need SDL headers, libraries, or `SDL2.dll`.
- **PPU rendering updates:** The scanline renderer selects the first eight vertically visible sprites in OAM order, follows the NES per-scanline sprite limit, and caches sprite-row decoding and opaque-pixel results. Background and Sprite 0 hit updates remain tied to PPU dot progression.
- **Audio and timing fixes:** V3.3 corrected DPCM byte-boundary and silence-bit timing, uses mapper-aware sample reads, and advances APU timing with the CPU instruction's base cycles. V3.4 moved audio synthesis and mixing to approximate integer arithmetic, aligned the audio clock to the emulator's nominal 60 fps (1,786,840 cycles/s), and set a 50 ms WinMM queue. The Windows builds use dual-timer frame pacing.
- **ROM loading:** V1.X keeps a native file picker and small command-line reader. V3.5 adds x86 support for single-file, single-stream LZMA1 `.7z` ROMs.

The audio mixer is an approximation: it does not implement the NES nonlinear TND mixer, and DMC DMA timing is not cycle-perfect.

## Version lines

### V1.X — V1.7

V1.7 focuses on a small Windows executable. It keeps the ROM file picker and accepts one simple ROM filename on the command line, resolved from the current working directory. Quoted paths containing spaces are not supported. Frame pacing uses two 16 ms waits followed by synchronization to a 50 ms boundary.

Builds use Clang `-Oz`; x64 builds also use LTO.

| Output | Size |
| --- | ---: |
| x86 Clang + LLD | 9,216 B |
| x86 Clang + Crinkler 3.0b FAST | 4,565 B |
| x64 Clang + LLD | 11,264 B |
| x64 Clang + UPX | 8,192 B |

The V1.X source and release files are maintained on the [`v1.x-windows-native` branch](https://github.com/stonechs/smolnes/tree/v1.x-windows-native).

### V3

V3.2 moved audio output to WinMM window callbacks. V3.3 added DMC playback. V3.4 uses integer audio synthesis and mixing, a 60 fps audio clock, and a 50 ms playback queue while retaining dual-timer frame pacing.

V3.5 adds x86 support for single-file, single-stream LZMA1 `.7z` ROMs. x64 continues to load uncompressed `.nes` ROMs only. The headless regression passed 116 LZMA1 archives in both debug and release paths; 3 unsupported methods were skipped.

| V3.5 output | Size |
| --- | ---: |
| x86 Clang + LLD | 16,384 B |
| x86 Clang + Crinkler | 8,431 B |
| x64 Clang + LLD | 17,920 B |
| x64 Clang + UPX | 12,288 B |

The four V3.5 executables are available from the [GitHub Release](https://github.com/stonechs/smolnes/releases/tag/v3.5). Its source snapshot, build files, maps, regression results, release files, and hashes are in [`dist/V3.5/final/`](dist/V3.5/final/). The V3.4 baseline package is in [`dist/V3.4/final/`](dist/V3.4/final/).

V3 builds use Clang `-Oz` for x86 and Clang `-Oz -flto` for x64; x86 uses Crinkler and x64 uses UPX for the packed outputs.

## Run

Start the executable without arguments to open the ROM picker, or pass one simple ROM path:

```text
smolnes-v3.5-clang-x86.exe game.nes
```

The command-line reader does not handle quoted paths with spaces. In the V3.5 x86 build, the file picker also lists `.7z` files.

| NES control | Key |
| --- | --- |
| D-pad | Arrow keys |
| B | Z |
| A | X |
| Start | Enter |
| Select | Tab |

## Build

The current source of truth is `src/`. Build all four V3.5 outputs from the repository root with:

```text
dist\V3.5\final\build-v3.5.cmd
```

The Windows build uses Clang/LLD, Visual Studio 2022 Build Tools and Windows SDK, NASM for the x86 LZMA decoder, Crinkler for x86 compression, and UPX for x64 compression. The V3.5 build script compiles the decoder source directly from `src/lzmacapi/SRC/`.

## Repository layout

- `src/` — current Windows-native source.
- `scripts/` — smoke tests and build helpers.
- `dist/` — versioned source snapshots, build records, and executables.
- `docs/` — project status, decisions, environment, and experiment notes.
- `roms/` — local ROM test files; ROMs are not bundled with releases.
- `tools/` — local build tools.
