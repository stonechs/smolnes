# smolnes Windows-native V1.2

V1.2 is a Windows-native build of smolnes based on `deobfuscated.c`. It uses Win32 for keyboard input and window handling, GDI for display, and a scanline-oriented sprite path. SDL is not required.

## Run

Download `smolnes-v1.2.exe` from the [V1.2 GitHub release](https://github.com/stonechs/smolnes/releases/tag/v1.2), then run it with a ROM path:

```powershell
.\smolnes-v1.2.exe "C:\path\to\game.nes"
```

No ROM files are bundled. The included `.nes` files under `fixtures/` are synthetic test fixtures only.

## Build

Install a MinGW-w64 GCC toolchain and run:

```powershell
.\build-v1.2.ps1 -GccPath "C:\path\to\mingw64\bin\gcc.exe"
```

The script writes `dist\V1.2\smolnes-v1.2.exe`. The build uses `-Os`, function/data section collection, and stripped symbols. The tested build is 29,184 bytes.

## What changed

- Selects the first eight vertically visible sprites in OAM order for each scanline.
- Decodes the selected sprites' pattern rows once per scanline and caches the first opaque sprite pixel at each screen x coordinate.
- Keeps background sampling and Sprite 0 hit updates tied to the existing PPU dot progression.
- Uses the existing 60 Hz frame pacer and Win32/GDI output.

## Verification

`pig.NES` opened, displayed the expected center pixel (`0x8C0000`), and closed normally. The sprite-limit fixture produced `0x636163`; the sprite-row fixture produced `0xFF9A63`. Performance CSVs are included in this package. The [scanline migration plan](../../../docs/plans/PPU_SCANLINE_MIGRATION.md) is maintained under the repository documentation. These checks do not establish broad ROM compatibility.
