# Deferred audio fidelity work

These items are intentionally deferred from the V2 release:

1. **Band-limited pulse and triangle waveforms (PolyBLEP or wavetable).** The current step-generated pulse and triangle can alias above the 48 kHz output Nyquist limit and sound gritty compared with the browser oscillators in jsnes-lite.
2. **Timestamped audio control events.** C currently snapshots shared voice controls while rendering 10 ms blocks, with about 60 ms of queued PCM. jsnes-lite schedules Web Audio parameter changes against APU time. An event queue would preserve intermediate register changes and improve timing alignment.
3. **DMC channel.** DMC playback, DMA stalls, and its IRQ behavior remain unsupported.
4. **NES analog-style mixer and filters.** The current mixer sums voices linearly and clips; it does not model the nonlinear pulse/TND mixer or analog filters.

PolyBLEP/band-limited waveform work is the first planned follow-up after V2.
