# V3.3 release notes

- Added NES DMC 1-bit DPCM playback on the V3.2 WinMM audio callback path.
- Corrected DPCM output-unit byte-boundary handling and silence bit counting.
- Kept mapper-aware sample reads and added timestamped DAC output events to the PCM mixer.
- Set the experimental linear DMC gain to 0.50 based on the user's listening comparison; no nonlinear TND mixer or integer conversion is included.
- Preserved the six-block/60 ms audio queue, window-message refill, controls, ROM selection, and V1.7-style dual-timer pacing.
- DMA timing remains a four-cycle boundary approximation and is not cycle-perfect.
