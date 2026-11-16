# V2 release notes

V2 is the Windows-native audio release based on the V1.2 scanline renderer.

- Added WinMM PCM playback with four software-generated APU channels.
- Added Pulse duty/envelope/sweep/length state, Triangle linear/length counters, Noise mode/envelope/length state, and CPU-cycle-clocked four/five-step frame sequencing.
- Matched gain transitions to approximately 1 ms and fixed Pulse 1 negative-sweep handling.
- No SDL runtime or ROMs are bundled.

## Validation

- `pig.NES` window pixel-grid smoke passed.
- APU register smoke passed: 499 PCM blocks, zero underruns.
- Pulse 1 negative-sweep regression passed at 224 Hz.
- Waveform, duty, noise-mode, and phase-continuity checks passed.

V2 is frozen with the remaining audio fidelity work listed in `DEFERRED.md`.
