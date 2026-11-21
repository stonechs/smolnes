#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <string.h>
#include "audio_win32.h"

#define AUDIO_SAMPLE_RATE 48000
#define AUDIO_CHANNELS 2
#define AUDIO_BLOCK_FRAMES 480
#define AUDIO_BLOCK_COUNT 6
#define AUDIO_MAX_FREQUENCY 18000.0
#define AUDIO_MAX_GAIN 0.50

typedef struct {
  int waveform;
  double frequency;
  double gain;
  unsigned duty;
  unsigned noise_mode;
} VoiceControl;

typedef struct {
  int waveform;
  double phase;
  double frequency;
  double gain;
  unsigned long noise_lfsr;
} VoiceRuntime;

static HWAVEOUT audio_device;
static VoiceControl audio_controls[AUDIO_VOICE_COUNT];
static VoiceRuntime audio_runtime[AUDIO_VOICE_COUNT];
static WAVEHDR audio_headers[AUDIO_BLOCK_COUNT];
static short audio_buffers[AUDIO_BLOCK_COUNT][AUDIO_BLOCK_FRAMES * AUDIO_CHANNELS];

static void audio_render_frames(short *output, unsigned frame_count) {
  VoiceControl targets[AUDIO_VOICE_COUNT];
  unsigned i, frame;

  for (i = 0; i < AUDIO_VOICE_COUNT; i++)
    targets[i] = audio_controls[i];

  for (frame = 0; frame < frame_count; frame++) {
    double mixed = 0.0;
    for (i = 0; i < AUDIO_VOICE_COUNT; i++) {
      VoiceRuntime *voice = &audio_runtime[i];
      double sample = 0.0;
      voice->waveform = targets[i].waveform;
      voice->frequency += (targets[i].frequency - voice->frequency) * 0.02;
      /* Match Web Audio setTargetAtTime(..., 0.001): about a 1 ms time constant. */
      voice->gain += (targets[i].gain - voice->gain) * 0.0206;

      if (voice->gain > 0.00001 && voice->frequency > 0.0) {
        if (voice->waveform == AUDIO_VOICE_SQUARE) {
          unsigned duty = targets[i].duty & 3;
          static const double duty_cycle[4] = {0.125, 0.25, 0.5, 0.75};
          sample = voice->phase < duty_cycle[duty] ? 1.0 : -1.0;
          voice->phase += voice->frequency / AUDIO_SAMPLE_RATE;
          if (voice->phase >= 1.0)
            voice->phase -= 1.0;
        } else if (voice->waveform == AUDIO_VOICE_TRIANGLE) {
          sample = voice->phase < 0.5 ? 4.0 * voice->phase - 1.0
                                      : 3.0 - 4.0 * voice->phase;
          voice->phase += voice->frequency / AUDIO_SAMPLE_RATE;
          if (voice->phase >= 1.0)
            voice->phase -= 1.0;
        } else {
          voice->phase += voice->frequency / AUDIO_SAMPLE_RATE;
          while (voice->phase >= 1.0) {
            unsigned long feedback =
                (voice->noise_lfsr ^
                 (voice->noise_lfsr >> (targets[i].noise_mode ? 6 : 1))) & 1UL;
            voice->noise_lfsr = (voice->noise_lfsr >> 1) | (feedback << 14);
            voice->phase -= 1.0;
          }
          sample = (voice->noise_lfsr & 1UL) ? 1.0 : -1.0;
        }
      }
      mixed += sample * voice->gain;
    }

    if (mixed > 1.0)
      mixed = 1.0;
    else if (mixed < -1.0)
      mixed = -1.0;
    output[frame * 2] = (short)(mixed * 32767.0);
    output[frame * 2 + 1] = output[frame * 2];
  }
}

static void audio_render_block(short *output) {
  audio_render_frames(output, AUDIO_BLOCK_FRAMES);
}

static void audio_release_device(void) {
  unsigned i;
  if (audio_device) {
    waveOutReset(audio_device);
    for (i = 0; i < AUDIO_BLOCK_COUNT; i++) {
      if (audio_headers[i].dwFlags & WHDR_PREPARED)
        waveOutUnprepareHeader(audio_device, &audio_headers[i],
                               sizeof(audio_headers[i]));
    }
    waveOutClose(audio_device);
    audio_device = NULL;
  }
}

int audio_start(void *callback_window) {
  WAVEFORMATEX format;
  unsigned i;
  MMRESULT result;

  if (audio_device)
    return 1;

  memset(audio_controls, 0, sizeof(audio_controls));
  memset(audio_runtime, 0, sizeof(audio_runtime));
  memset(audio_headers, 0, sizeof(audio_headers));
  for (i = 0; i < AUDIO_VOICE_COUNT; i++) {
    audio_controls[i].waveform = i == 2 ? AUDIO_VOICE_TRIANGLE
                                        : AUDIO_VOICE_SQUARE;
    audio_runtime[i].waveform = audio_controls[i].waveform;
    audio_runtime[i].noise_lfsr = 1;
  }
  memset(&format, 0, sizeof(format));
  format.wFormatTag = WAVE_FORMAT_PCM;
  format.nChannels = AUDIO_CHANNELS;
  format.nSamplesPerSec = AUDIO_SAMPLE_RATE;
  format.wBitsPerSample = 16;
  format.nBlockAlign = (WORD)(AUDIO_CHANNELS * sizeof(short));
  format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
  result = waveOutOpen(&audio_device, WAVE_MAPPER, &format,
                       (DWORD_PTR)callback_window, 0, CALLBACK_WINDOW);
  if (result != MMSYSERR_NOERROR)
    goto fail;

  for (i = 0; i < AUDIO_BLOCK_COUNT; i++) {
    WAVEHDR *header = &audio_headers[i];
    header->lpData = (LPSTR)audio_buffers[i];
    header->dwBufferLength = sizeof(audio_buffers[i]);
    if (waveOutPrepareHeader(audio_device, header, sizeof(*header)) !=
        MMSYSERR_NOERROR)
      goto fail;
    audio_render_block(audio_buffers[i]);
    if (waveOutWrite(audio_device, header, sizeof(*header)) != MMSYSERR_NOERROR)
      goto fail;
  }

  return 1;

fail:
  audio_release_device();
  return 0;
}

void audio_set_voice(unsigned voice, int waveform, double frequency,
                     double gain) {
  if (voice >= AUDIO_VOICE_COUNT)
    return;
  if (waveform < AUDIO_VOICE_SQUARE || waveform > AUDIO_VOICE_NOISE)
    waveform = AUDIO_VOICE_SQUARE;
  if (frequency < 0.0)
    frequency = 0.0;
  if (frequency > (waveform == AUDIO_VOICE_NOISE ? 1000000.0
                                                  : AUDIO_MAX_FREQUENCY))
    frequency = waveform == AUDIO_VOICE_NOISE ? 1000000.0
                                               : AUDIO_MAX_FREQUENCY;
  if (gain < 0.0)
    gain = 0.0;
  if (gain > AUDIO_MAX_GAIN)
    gain = AUDIO_MAX_GAIN;

  audio_controls[voice].waveform = waveform;
  audio_controls[voice].frequency = frequency;
  audio_controls[voice].gain = gain;
}

void audio_set_pulse_duty(unsigned voice, unsigned duty) {
  if (voice >= 2)
    return;
  audio_controls[voice].duty = duty & 3;
}

void audio_set_noise_mode(unsigned mode) {
  audio_controls[3].noise_mode = mode != 0;
}

void audio_handle_done(void *completed_header) {
  WAVEHDR *header = (WAVEHDR *)completed_header;
  if (!audio_device || !(header->dwFlags & WHDR_DONE) ||
      (header->dwFlags & WHDR_INQUEUE))
    return;
  audio_render_block((short *)header->lpData);
  waveOutWrite(audio_device, header, sizeof(*header));
}

void audio_stop(void) {
  audio_release_device();
}

#ifdef AUDIO_TESTING
void audio_test_reset(void) {
  unsigned i;
  memset(audio_controls, 0, sizeof(audio_controls));
  memset(audio_runtime, 0, sizeof(audio_runtime));
  for (i = 0; i < AUDIO_VOICE_COUNT; i++)
    audio_runtime[i].noise_lfsr = 1;
}

void audio_test_render(short *output, unsigned frame_count) {
  audio_render_frames(output, frame_count);
}
#endif
