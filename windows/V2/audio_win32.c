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
static HANDLE audio_event;
static HANDLE audio_thread_handle;
static CRITICAL_SECTION audio_control_lock;
static int audio_lock_initialized;
static volatile LONG audio_running;
static volatile LONG audio_completed_count;
static volatile LONG audio_underrun_count;
static VoiceControl audio_controls[AUDIO_VOICE_COUNT];
static VoiceRuntime audio_runtime[AUDIO_VOICE_COUNT];
static WAVEHDR audio_headers[AUDIO_BLOCK_COUNT];
static short audio_buffers[AUDIO_BLOCK_COUNT][AUDIO_BLOCK_FRAMES * AUDIO_CHANNELS];

static void audio_render_frames(short *output, unsigned frame_count) {
  VoiceControl targets[AUDIO_VOICE_COUNT];
  unsigned i, frame;

  if (audio_lock_initialized)
    EnterCriticalSection(&audio_control_lock);
  for (i = 0; i < AUDIO_VOICE_COUNT; i++)
    targets[i] = audio_controls[i];
  if (audio_lock_initialized)
    LeaveCriticalSection(&audio_control_lock);

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

static DWORD WINAPI audio_worker(LPVOID unused) {
  unsigned i;
  (void)unused;

  while (InterlockedCompareExchange(&audio_running, 1, 1)) {
    DWORD wait_result = WaitForSingleObject(audio_event, INFINITE);
    if (wait_result != WAIT_OBJECT_0 ||
        !InterlockedCompareExchange(&audio_running, 1, 1))
      break;

    {
      unsigned completed = 0;
      for (i = 0; i < AUDIO_BLOCK_COUNT; i++)
        if ((audio_headers[i].dwFlags & WHDR_DONE) &&
            !(audio_headers[i].dwFlags & WHDR_INQUEUE))
          completed++;
      if (completed == AUDIO_BLOCK_COUNT)
        InterlockedIncrement(&audio_underrun_count);
    }

    for (i = 0; i < AUDIO_BLOCK_COUNT; i++) {
      WAVEHDR *header = &audio_headers[i];
      if ((header->dwFlags & WHDR_DONE) &&
          !(header->dwFlags & WHDR_INQUEUE)) {
        InterlockedIncrement(&audio_completed_count);
        audio_render_block((short *)header->lpData);
        if (waveOutWrite(audio_device, header, sizeof(*header)) != MMSYSERR_NOERROR)
          InterlockedIncrement(&audio_underrun_count);
      }
    }
  }
  return 0;
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

int audio_start(void) {
  WAVEFORMATEX format;
  unsigned i;
  MMRESULT result;

  if (audio_device)
    return 1;

  InitializeCriticalSection(&audio_control_lock);
  audio_lock_initialized = 1;
  memset(audio_controls, 0, sizeof(audio_controls));
  memset(audio_runtime, 0, sizeof(audio_runtime));
  memset(audio_headers, 0, sizeof(audio_headers));
  for (i = 0; i < AUDIO_VOICE_COUNT; i++) {
    audio_controls[i].waveform = i == 2 ? AUDIO_VOICE_TRIANGLE
                                        : AUDIO_VOICE_SQUARE;
    audio_runtime[i].waveform = audio_controls[i].waveform;
    audio_runtime[i].noise_lfsr = 1;
  }
  InterlockedExchange(&audio_completed_count, 0);
  InterlockedExchange(&audio_underrun_count, 0);

  audio_event = CreateEventA(NULL, FALSE, FALSE, NULL);
  if (!audio_event)
    goto fail;

  memset(&format, 0, sizeof(format));
  format.wFormatTag = WAVE_FORMAT_PCM;
  format.nChannels = AUDIO_CHANNELS;
  format.nSamplesPerSec = AUDIO_SAMPLE_RATE;
  format.wBitsPerSample = 16;
  format.nBlockAlign = (WORD)(AUDIO_CHANNELS * sizeof(short));
  format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
  result = waveOutOpen(&audio_device, WAVE_MAPPER, &format,
                       (DWORD_PTR)audio_event, 0, CALLBACK_EVENT);
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

  InterlockedExchange(&audio_running, 1);
  audio_thread_handle = CreateThread(NULL, 0, audio_worker, NULL, 0, NULL);
  if (!audio_thread_handle)
    goto fail;
  return 1;

fail:
  InterlockedExchange(&audio_running, 0);
  if (audio_event)
    CloseHandle(audio_event);
  audio_event = NULL;
  audio_release_device();
  if (audio_lock_initialized) {
    DeleteCriticalSection(&audio_control_lock);
    audio_lock_initialized = 0;
  }
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

  if (audio_lock_initialized)
    EnterCriticalSection(&audio_control_lock);
  audio_controls[voice].waveform = waveform;
  audio_controls[voice].frequency = frequency;
  audio_controls[voice].gain = gain;
  if (audio_lock_initialized)
    LeaveCriticalSection(&audio_control_lock);
}

void audio_set_pulse_duty(unsigned voice, unsigned duty) {
  if (voice >= 2)
    return;
  if (audio_lock_initialized)
    EnterCriticalSection(&audio_control_lock);
  audio_controls[voice].duty = duty & 3;
  if (audio_lock_initialized)
    LeaveCriticalSection(&audio_control_lock);
}

void audio_set_noise_mode(unsigned mode) {
  if (audio_lock_initialized)
    EnterCriticalSection(&audio_control_lock);
  audio_controls[3].noise_mode = mode != 0;
  if (audio_lock_initialized)
    LeaveCriticalSection(&audio_control_lock);
}

void audio_stop(void) {
  InterlockedExchange(&audio_running, 0);
  if (audio_event)
    SetEvent(audio_event);
  if (audio_thread_handle) {
    WaitForSingleObject(audio_thread_handle, INFINITE);
    CloseHandle(audio_thread_handle);
    audio_thread_handle = NULL;
  }
  audio_release_device();
  if (audio_event) {
    CloseHandle(audio_event);
    audio_event = NULL;
  }
  if (audio_lock_initialized) {
    DeleteCriticalSection(&audio_control_lock);
    audio_lock_initialized = 0;
  }
}

unsigned long audio_completed_blocks(void) {
  return (unsigned long)InterlockedCompareExchange(&audio_completed_count, 0, 0);
}

unsigned long audio_underruns(void) {
  return (unsigned long)InterlockedCompareExchange(&audio_underrun_count, 0, 0);
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
