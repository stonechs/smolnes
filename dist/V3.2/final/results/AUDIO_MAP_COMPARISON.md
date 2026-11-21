# V3.1 WinMM Window Callback Size Comparison

## x86 LLD map function ranges

Ranges use the next mapped function symbol as the end. They can include a small amount of alignment and constants.

| Function | Official V3.1 | Callback candidate | Size change |
|---|---:|---:|---:|
| `audio_start` | 0x23CC | 0x23E8 | 485 B -> 306 B |
| `audio_render_block` | 0x25B1 | 0x251A | 618 B -> 578 B |
| `audio_worker` | 0x281B | removed | 194 B -> 0 |
| `audio_release_device` | 0x28DD | 0x275C | 87 B -> 87 B |
| `audio_set_voice` | 0x2934 | 0x27B3 | 204 B -> 100 B |
| `audio_set_pulse_duty` | 0x2A00 | 0x2817 | 84 B -> 26 B |
| `audio_set_noise_mode` | 0x2A54 | 0x2831 | 74 B -> 16 B |
| `audio_handle_done` | — | 0x2841 | 0 B -> 49 B |
| `audio_stop` | 0x2A9E | 0x2872 | 117 B -> 5 B |
| Approx. audio function span | 1,863 B | 1,167 B | -696 B |

The callback version removes `CreateThread`, `CreateEventA`, `WaitForSingleObject` (audio worker use), `SetEvent`, and audio critical-section calls from the audio path. `CALLBACK_WINDOW` posts `MM_WOM_DONE` to the existing HWND; the window procedure processes the completed header and calls `waveOutWrite` on the main thread. The render loop and WinMM open/prepare/write/reset/close APIs remain.

## Artifacts

| Artifact | Size |
|---|---:|
| x86 LLD | 14,336 B |
| x86 Crinkler | 6,954 B |
| x64 LLD | 17,408 B |
| x64 UPX 5.3.0 | 11,776 B |

`release/SHA256SUMS.txt` records hashes. No runtime audio test was run; this is not a replacement for the official V3.1 build.
