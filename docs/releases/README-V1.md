# smolnes Windows-native V1

V1 keeps the readable emulator core and replaces SDL with Win32 keyboard/window handling and GDI rendering. It uses the original BGR565 framebuffer layout and needs no `SDL2.dll`.

## Build snapshot

V1 is a frozen release. Its published executable is available under `dist/V1/`. The separate source/build snapshot was removed from the V1.x branch; original upstream SDL sources remain under `src/org/`.

## Run

```powershell
.\dist\V1\smolnes-v1.exe "C:\path\to\game.nes"
```

The executable uses Windows system components (GDI32, USER32, KERNEL32, and the Universal CRT). It does not need SDL runtime files beside it.

The included V1 binary was smoke-checked with a generated NROM and `roms/commercial/pig.NES`. That verifies startup and frame output only; it does not establish broad game compatibility.
