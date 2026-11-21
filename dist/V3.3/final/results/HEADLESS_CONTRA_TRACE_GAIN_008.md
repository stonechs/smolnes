# Contra headless DMC execution trace

Date: 2026-11-21. Scope: isolated V3.2+DMC experiment source, the supplied `roms/commercial/魂斗罗.nes`, x86 MSVC diagnostic build. No GUI window was created and no sound device was opened.

## Run method

The diagnostic build runs the emulator's actual ROM loader, CPU instruction loop, PPU dot loop, mapper-2 PRG access, APU and audio PCM renderer for 1,200 emulated frames. It skips window creation and frame timers. It injects two short Start key presses at frames 120 and 180 to proceed from the title/menu into gameplay. The audio renderer is advanced by 800 PCM frames per emulated frame after mirroring its normal six-buffer prefill.

## Observed DMC activity

- ROM writes `$4010=0F`, `$4011=2F`, `$4012=F0`, `$4013=05`, then enables DMC in `$4015=1F`. This selects a short sample at `$FC00`, 81 bytes, at the fastest DMC rate.
- Around CPU cycle 7,413,724 (about frame 249), the stream changes to `$4011=75`, `$4012=F3`, `$4013=25`; that selects `$FCC0`, length 593 bytes, used for the longer percussion sample.
- After correcting the output-unit byte boundary against NESdev and Mesen2, the rerun recorded 7,277 actual mapper-aware DMC reads, including repeated bursts from `$FC00` and `$FCC0`. Every logged byte matches the ROM byte at the mapped file offset (zero mismatches). 430 fetched bytes (5.91%) are exactly `$00`; this is a whole-byte count, not a bit ratio. The 58,216 fetched bits contain 31,055 zero bits (53.34%) and 27,161 one bits (46.66%).
- The first `$FCC0` playback reads 145 sequential bytes (`$FCC0-$FD50`), exactly matching the first 145 bytes of the 593-byte sample. The game writes `$4015=0F` and restarts DMC before that sample's configured length finishes; later `$FCC0` starts also appear in the trace. Therefore the isolated A/B WAVs play the complete 593-byte sample, while the whole-game log shows the actual shorter/retriggered playback segments.
- The 20-second isolated DMC PCM ranged from -2,621 to 1,761, with mean -1,423.84 and RMS 1,717.24 (16-bit samples). The measured maximum absolute DMC component was 2,621/32767. Total PCM block peak reached 6,893/32767.
- A separate APU unit test uses byte `$A5` and confirms its low-to-high bit playback produces DAC levels `66, 64, 66, 64, 62, 64, 62, 64` from an initial level of 64.

## Artifacts and limits

- `apu-register-trace.csv`: CPU-cycle-stamped APU writes.
- `apu-dmc-fetch.csv`: every DMC sample address/value fetched.
- `headless-pcm.csv`: per-30-frame total PCM peak/energy and DMC component peak.

This confirms the DMC configuration, ROM sample fetch, DPCM output unit and PCM mixing path all execute during the actual game CPU/PPU run. It does not capture sound through a physical audio device or establish subjective audibility. The fixed Start presses are inferred to have entered the stage from the longer gameplay percussion sample configuration; there is no rendered screen in the headless run.

The data fetches are not close to all-zero, and each fetched byte is passed through the Mapper 2 fixed-bank ROM reader. The remaining audible-level question is separate from DPCM bit decoding: the measured isolated PCM has a substantial negative mean/DC component, while its RMS is similar to the earlier capture. This points to the DAC-to-mixer scaling and DC handling as likely causes of the weak or click-like presentation; listening is still needed to establish the audible result.
