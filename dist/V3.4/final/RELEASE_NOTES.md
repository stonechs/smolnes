# V3.4 release notes

- Replaced floating-point audio synthesis/mixing with integer phase, frequency steps, Q15 gains, DMC cycle accumulation, mixing and clipping.
- Aligned pulse/noise pitch and DMC sample progression to the nominal 60fps frame clock (1,786,840 cycles/s).
- Set the WinMM audio queue to 50ms using five 10ms buffers.
- Preserved V3.3 DMC mapper-aware reads, 0.50 linear gain, callback refill, controls, ROM selection and dual-timer frame pacing.
- Integer coefficients are approximate. The mixer is not the NES nonlinear TND model; DMC DMA timing is not cycle-perfect.
- This local package has not been published. Runtime/audio verification of the V3.4 executables remains outstanding.
