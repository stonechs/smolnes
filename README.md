# smolnes

A NES emulator in ~5000 significant bytes of c.

## Features

Plays some mapper 0/1/2/3/4/7 games.

## Screenshots

![Batman](img/batman.gif)
![Kirby's Adventure](img/kirby.gif)
![Mega Man 3](img/mm3.gif)
![Little Nemo the Dream Master](img/nemo.gif)
![River City Ransom](img/rcr.gif)
![Super Mario Bros. 2](img/smb2.gif)
![Super Mario Bros. 3](img/smb3.gif)
![Teenage Mutant Ninja Turtles 2](img/tmnt2.gif)
![Bionic Commando](img/bc.gif)
![Donkey Kong](img/dk.gif)
![Dragon Warrior](img/dw.gif)
![Adventures of Lolo](img/lolo.gif)
![Legend of Zelda](img/loz.gif)
![Metroid](img/met.gif)
![Mega Man 2](img/mm2.gif)
![Ninja Gaiden](img/ng.gif)
![Rad Racer](img/rr.gif)
![Super Mario Bros.](img/smb.gif)
![Teenage Mutant Ninja Turtle](img/tmnt.gif)
![Contra](img/contra.gif)
![Goonies 2](img/g2.gif)
![Castlevania](img/cv.gif)
![DuckTales](img/dt.gif)
![Alwa's Awakening](img/alwa.gif)
![Witch n' Wiz](img/wnw.gif)
![Battletoads](img/bt.gif)
![Wizard's and Warriors II](img/ww2.gif)
![Gradius](img/grad.gif)

![Source Code](img/smolnes.png)

## Building

Probably only builds on Linux and macOS. Try gcc or clang.

```
$ make
```

## Running

```
$ ./smolnes <rom.nes>
```

Keys:

| Action | Key |
| --- | --- |
| DPAD-UP | <kbd>↑</kbd> |
| DPAD-DOWN | <kbd>↓</kbd> |
| DPAD-LEFT | <kbd>←</kbd> |
| DPAD-RIGHT | <kbd>→</kbd> |
| B | <kbd>Z</kbd> |
| A | <kbd>X</kbd> |
| START | <kbd>Enter</kbd> |
| SELECT | <kbd>Tab</kbd> |

## Updating keys

Look at `src/org/smolnes.c` around line 20. The following table shows which
numbers map to which keyboard keys:

| character | number | default key | NES button |
| - | - | - | - |
| \33 | 27 | X | A Button |
| \35 | 29 | Z | B Button |
| + | 43 | Tab | Select Button |
| ( | 40 | Return | Start Button |
| R | 82 | Arrow Up | DPAD Up |
| Q | 81 | Arrow Down | DPAD Down |
| P | 80 | Arrow Left | DPAD Left |
| O | 79 | Arrow Right | DPAD Right |

1. Look up the keys you want to replace from this list [SDL scancode list](https://github.com/libsdl-org/SDL/blob/SDL2/include/SDL_scancode.h)
1. Convert the number to its ascii character, or C character literal.
1. Replace this character in the string.

## Projects using smolnes

* [p2r3/smolnes-nut](https://github.com/p2r3/smolnes-nut) The smolnes emulator
  ported to Squirrel for Portal 2 - https://youtu.be/PUTRvF7slYs
* [nevesnunes/ghidra-plays-mario](https://github.com/nevesnunes/ghidra-plays-mario)
  Playing NES ROMs with Ghidra's PCode Emulator
* [sysprog21/rv32emu](https://github.com/sysprog21/rv32emu) Compact and
  Efficient RISC-V RV32I[MAFC] emulator (uses smolnes as a demo)
* [ma.tthias.xyz/en/blog/smolnes-vuln-research/](https://ma.tthias.xyz/en/blog/smolnes-vuln-research/)
  Vulnerability Research on SmolNES
* [cyklusiguess/hxNes](https://github.com/cyklusiguess/hxNes)
  smolnes emulator rewritten to Haxe using HaxeFlixel. Has sound and includes a debugging mode feature.
  

Let me know if you've used smolnes and I'll add it to the list.

## V1.x source layout

The original upstream SDL sources are in `src/org/`. The latest Windows-native V1.x source is under `dist/V1.6/candidate/source/`. The frozen V1 executable remains under `dist/V1/`; its separate build snapshot is no longer included in this branch.
## V1.4 candidate

The V1.4 dual-timer release candidate for the V1.x line is under `dist/V1.4/candidate/`. Only the extreme timer-size build is included; see [its release notes](docs/releases/README-V1.4.md). Runtime pacing and game compatibility have not yet been validated.
## V1.5 candidate

V1.5 retains the scanline selection of up to eight sprites from 64 and uses on-demand pixel handling. The four x64/x86 builds and reproducible inputs are under [dist/V1.5/candidate/](dist/V1.5/candidate/). See [V1.5 release notes](docs/releases/README-V1.5.md). Runtime/gameplay testing has not been performed.

## V1.6 release

V1.6 keeps the V1.5 emulator source and builds it with Clang/LLVM 23.1.5. The x86 packed asset uses Crinkler 3.0b and the x64 packed asset uses UPX 5.3.0. Four x86/x64 executables and their build inputs are under [dist/V1.6/candidate/](dist/V1.6/candidate/). See [V1.6 release notes](docs/releases/README-V1.6.md).

