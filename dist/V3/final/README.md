# smolnes Windows-native V3

V3 packages the Windows-native V2-final emulator with a smaller PE layout and a UPX-compressed x64 executable. The UI uses Win32/GDI and audio uses WinMM. SDL is not required.

## Run

```powershell
.\smolnes-v3.exe "C:\path\to\game.nes"
```

Arrows move, `X` is A, `Z` is B, Enter is Start, and Tab is Select. ROMs are not included.

## Build the packed executable

V3 was built with WinLibs MinGW-w64 GCC 16.2.0 x86_64 UCRT and UPX 5.3.0. With compatible tools installed:

```powershell
.\build-v3.ps1 -GccPath "C:\path\to\mingw64\bin\gcc.exe" -UpxPath "C:\path\to\upx.exe"
```

The script compiles with `-Os -flto`, custom entry point, and `-nostdlib`, then packs with `upx --best --lzma` and verifies the packed EXE. The release executable is the packed output. Build flags and PE details are in `results/BUILD_AND_SIZE.md`.
