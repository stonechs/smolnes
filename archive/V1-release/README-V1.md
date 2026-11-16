# smolnes Windows-native V1

This V1 keeps the readable emulator core from `deobfuscated.c` and replaces SDL with Win32 keyboard/window handling and GDI rendering. It uses the original BGR565 framebuffer layout and needs no `SDL2.dll`.

## Build

Use a 64-bit MinGW-w64 GCC toolchain on Windows. From PowerShell in this repository:

```powershell
.\build-v1.ps1 -GccPath "C:\path\to\mingw64\bin\gcc.exe"
```

The default output is `dist\V1\smolnes-v1.exe`. If `gcc.exe` is on `PATH`, `-GccPath` can be omitted.

## Run

```powershell
.\dist\V1\smolnes-v1.exe "C:\path\to\game.nes"
```

The executable uses Windows system components (GDI32, USER32, KERNEL32, and the Universal CRT). It does not need SDL runtime files beside it.

The included V1 binary was smoke-checked with a generated NROM and `roms/commercial/pig.NES`. That verifies startup and frame output only; it does not establish broad game compatibility.
