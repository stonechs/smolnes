Windows-native V1.2 based on `deobfuscated.c`, using Win32 window and keyboard handling with GDI output. No SDL runtime is required.

## Download and run

Download `smolnes-v1.2.exe`, then run it with a ROM path:

```powershell
.\smolnes-v1.2.exe "C:\path\to\game.nes"
```

The source package and SHA-256 file are attached. The source package contains the build script, source, synthetic test fixtures, smoke-test results, and per-stage performance CSVs. No commercial ROMs are included.

## Verification

`pig.NES` opened, rendered the expected center pixel (`0x8C0000`), and closed normally. Synthetic sprite-limit and sprite-row checks passed. The final executable is 29,184 bytes.

On the same machine and ROM, measured emulator work decreased from 10.830 ms/frame in the V1.1 baseline to 3.344 ms/frame in the final V1.2 candidate (600-frame samples). GDI timings and stage CSVs are included in the source package. These checks do not establish broad ROM compatibility.
