# smolnes Windows-native V1.1

V1.1 keeps the V1 Win32/GDI port and replaces the fixed per-frame sleep with a 60 Hz frame pacer. It uses a high-resolution waitable timer when Windows supports it, with a standard waitable timer fallback. Deadlines are calculated from `QueryPerformanceCounter`; missed deadlines are skipped, and Windows messages continue to be processed while waiting.

## Build

```powershell
.\build-v1.1.ps1 -GccPath "C:\path\to\mingw64\bin\gcc.exe"
```

The output is `dist\V1.1\smolnes-v1.1.exe`. Add `-FpsDiagnostics` to build a diagnostic executable that reports its measured frame rate in the window title.

## Run

```powershell
.\dist\V1.1\smolnes-v1.1.exe "C:\path\to\game.nes"
```

The executable has no SDL runtime dependency. The V1.1 build was smoke-checked with `pig.NES`: the window opened and closed cleanly, and its center pixel matched the expected nonblack color. On the same machine and ROM, diagnostic builds measured V1 at 27–31 FPS and V1.1 at 59–60 FPS after warm-up. An overrun diagnostic that adds 8 ms per frame measured 40–44 FPS, confirming the loop continues immediately after missed deadlines instead of waiting another full period. These checks do not establish broad game compatibility or frame pacing on every Windows system.
