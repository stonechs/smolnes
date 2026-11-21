# smolnes Windows-native V3.2

V3.2 carries forward the V3.1 Windows-native NES emulator, ROM selection, controls, dual waitable-timer frame pacing, Clang size-oriented builds, and WinMM PCM audio. The audio output now uses WinMM window notifications instead of an application-created audio worker thread.

## ROM loading

- With no ROM argument, the Windows Open File dialog appears. It filters `.nes` files and also allows all files. Cancel exits normally.
- A single simple, unquoted ROM argument is passed directly to `CreateFileA`. A filename is relative to the current working directory. Quoted paths, multiple arguments, and filenames containing spaces are not parsed.

## Controls

Arrow keys move; `X` is A, `Z` is B, Enter is Start, and Tab is Select.

## Four Windows outputs

| Output | Size |
|---|---:|
| smolnes-v3.2-clang-x64-upx.exe | 11,776 B |
| smolnes-v3.2-clang-x64.exe | 17,408 B |
| smolnes-v3.2-clang-x86-crinkler.exe | 6,954 B |
| smolnes-v3.2-clang-x86.exe | 14,336 B |

The x86 Crinkler and x64 UPX outputs are the packed builds. SHA-256 hashes are in `release/SHA256SUMS.txt`.

## Audio scheduling change

Each block remains 480 stereo frames at 48 kHz (10 ms), with six blocks queued (60 ms). WinMM posts `MM_WOM_DONE` to the existing window when a block completes; the window procedure refills and requeues it on the emulator's main thread. The application-created worker thread, event, audio critical section, and thread/stat counters were removed. The PCM synthesis and mixing code was retained.

Because audio refills now run through the window message pump, long delays in the emulator's main thread could postpone requeueing. The release binaries were not launched for runtime audio/gameplay validation; listen and check playback before treating the scheduling change as runtime-verified.

## Rebuild

Run `build-v3.2.cmd` with Visual Studio 2022 Build Tools/Windows SDK, Clang/LLD 23.1.5, Crinkler 3.0b at `tools/crinkler/`, and UPX 5.3.0 at `tools/upx.exe`. x86 uses Clang `-Oz` COFF objects for LLD and Crinkler; x64 uses Clang `-Oz -flto` and LLD. x86 uses Crinkler FAST mode with hash size 32, 100 tries, and CALLS transform; x64 uses UPX `--brute`. See `results/BUILD_AND_SIZE.md`.
