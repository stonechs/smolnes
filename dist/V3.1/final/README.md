# smolnes Windows-native V3.1

V3.1 is based on the V3 Windows-native emulator and uses Clang/LLVM 23.1.5. It carries forward the V1.7 dual waitable-timer frame pacing and ROM selection flow. V3.1 keeps the existing Win32/GDI renderer and WinMM audio. No ROM is included.

## ROM loading

- With no ROM argument, the Windows Open File dialog appears. It filters `.nes` files and also allows all files. Cancel exits normally.
- A single simple, unquoted ROM argument is passed directly to `CreateFileA`. A filename is relative to the current working directory. Quoted paths, multiple arguments, and filenames containing spaces are not parsed.

## Controls

Arrow keys move; `X` is A, `Z` is B, Enter is Start, and Tab is Select.

## Four Windows outputs

| Output | Size |
|---|---:|
| smolnes-v3.1-clang-x64-upx.exe | 12,288 B |
| smolnes-v3.1-clang-x64.exe | 17,920 B |
| smolnes-v3.1-clang-x86-crinkler.exe | 7,276 B |
| smolnes-v3.1-clang-x86.exe | 14,848 B |

The x86 Crinkler output and x64 UPX output are the packed releases. SHA-256 hashes are in `release/SHA256SUMS.txt`.

## Rebuild

Run `build-v3.1.cmd` from any working directory with Visual Studio 2022 Build Tools/Windows SDK, user-PATH Clang 23.1.5, Crinkler 3.0b at `tools/crinkler/`, and UPX at `tools/upx.exe` available. The script builds x86 Clang `-Oz` COFF objects for LLD and Crinkler; x64 uses Clang `-Oz -flto` and LLD. x86 is compressed with Crinkler 3.0b (`FAST`, hash size 32, 100 tries, CALLS transform); x64 is compressed with UPX 5.3.0 `--brute`. See `results/BUILD_AND_SIZE.md` for measured outputs and tool flags.

The EXEs were built, their PE architectures were inspected, the UPX artifact passed `upx -t`, and SHA-256 values were verified. The binaries were not launched; ROM picker interaction, gameplay, and real frame cadence have not been runtime-verified.
