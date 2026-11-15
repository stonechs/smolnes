# V2 smoke-test results

Build toolchain: WinLibs MinGW-w64 GCC 16.2.0.

- `smolnes-v2.exe`: 33,792 bytes; static MinGW runtime, `-Os`, stripped symbols, and function-section collection.
- `pig.NES`: window opened; 13,440/13,440 sampled window-client pixels were nonblack; clean close.
- APU register/waveOut smoke: PASS; 500 PCM blocks completed; zero underruns.
- Pulse 1 negative-sweep regression: PASS; 224 positive crossings/second at timer `$01F1`, both waveform polarities present.
- Software waveform tests: PASS; square 439 and triangle 440 crossings/second; 12.5% and 50% duty distinguishable; long/short noise modes differ; phase continuity passed.
- Separate three-second `pig.NES` audio diagnostic: 300 PCM blocks, zero underruns.

Commercial ROMs were tested locally but are not included. The fixture under `fixtures/` is synthetic.
