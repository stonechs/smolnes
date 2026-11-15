# smolnes Windows-native V2

V2 adds native Windows audio to the V1.2 Windows build. It uses Win32/GDI for the window and display, WinMM `waveOut` for 48 kHz PCM, and GCC/MinGW-w64. SDL is not required.

## Run

```powershell
.\smolnes-v2.exe "C:\path\to\game.nes"
```

Keys: arrows move, `X` is A, `Z` is B, Enter is Start, and Tab is Select. ROMs are not included. `fixtures\apu-registers.nes` is a synthetic test fixture.

## Build

Install WinLibs MinGW-w64 GCC 16.2.0 or another compatible GCC and run:

```powershell
.\build-v2.ps1 -GccPath "C:\path\to\mingw64\bin\gcc.exe"
```

The script builds a static MinGW executable (with Windows system libraries) using `-Os`, function-section garbage collection, and stripped symbols.

## Audio scope

Implements Pulse 1/2, Triangle, and Noise with duty, envelope, sweep, length/linear counters, noise modes, and the frame sequencer. DMC is not implemented. Known follow-up work is recorded in [DEFERRED.md](DEFERRED.md).
