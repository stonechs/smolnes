#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <string.h>
#include <stdint.h>
#include "audio_win32.h"
#ifdef SMOLNES_AUDIO_DMC_DIAGNOSTICS
#include <stdio.h>
#endif

#define AUDIO_SAMPLE_RATE 48000
#define AUDIO_CHANNELS 2
#define AUDIO_BLOCK_FRAMES 480
#define AUDIO_BLOCK_COUNT 6
#define AUDIO_DMC_EVENT_COUNT 4096
#define AUDIO_DMC_PREROLL_CPU_CYCLES \
  ((double)AUDIO_BLOCK_COUNT * AUDIO_BLOCK_FRAMES * 1789773.0 / \
   AUDIO_SAMPLE_RATE)
#define AUDIO_DMC_PREROLL_CYCLES ((uint32_t)AUDIO_DMC_PREROLL_CPU_CYCLES)
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

static uint32_t audio_dmc_event_cycles[AUDIO_DMC_EVENT_COUNT];
static unsigned char audio_dmc_event_levels[AUDIO_DMC_EVENT_COUNT];
static uint32_t audio_dmc_head, audio_dmc_tail;
static uint32_t audio_dmc_cycle;
static unsigned audio_dmc_level = 64;
static double audio_dmc_cycle_fraction;
#ifdef SMOLNES_AUDIO_DMC_DIAGNOSTICS
static uint32_t audio_dmc_overflows, audio_dmc_processed;
static unsigned audio_dmc_render_blocks;
static FILE *audio_dmc_diag_file;
static FILE *audio_dmc_enqueue_file;
#endif
#ifdef AUDIO_TESTING
static int audio_test_last_dmc_peak;
static short *audio_test_dmc_output;
#endif

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

    {
      double cycle_count = audio_dmc_cycle_fraction +
                           1789773.0 / AUDIO_SAMPLE_RATE;
      unsigned cpu_cycles = (unsigned)cycle_count;
      audio_dmc_cycle_fraction = cycle_count - cpu_cycles;
      audio_dmc_cycle += cpu_cycles;
      while (audio_dmc_tail != audio_dmc_head &&
             (int32_t)(audio_dmc_event_cycles[
                           audio_dmc_tail & (AUDIO_DMC_EVENT_COUNT - 1)] -
                       audio_dmc_cycle) <= 0) {
        audio_dmc_level = audio_dmc_event_levels[
            audio_dmc_tail & (AUDIO_DMC_EVENT_COUNT - 1)];
        audio_dmc_tail++;
#ifdef SMOLNES_AUDIO_DMC_DIAGNOSTICS
        audio_dmc_processed++;
#endif
      }
      double dmc_sample = ((int)audio_dmc_level - 64) * (0.50 / 64.0);
#ifdef AUDIO_TESTING
      {
        int dmc_peak = (int)(dmc_sample < 0 ? -dmc_sample * 32767.0
                                             : dmc_sample * 32767.0);
        if (dmc_peak > audio_test_last_dmc_peak)
          audio_test_last_dmc_peak = dmc_peak;
        if (audio_test_dmc_output) {
          short dmc_pcm = (short)(dmc_sample * 32767.0);
          audio_test_dmc_output[frame * 2] = dmc_pcm;
          audio_test_dmc_output[frame * 2 + 1] = dmc_pcm;
        }
      }
#endif
      mixed += dmc_sample;
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

void audio_dmc_reset(unsigned output_level) {
  audio_dmc_head = audio_dmc_tail = 0;
  /* apu_reset runs after audio_start has queued the initial 60 ms of PCM. */
  audio_dmc_cycle = AUDIO_DMC_PREROLL_CYCLES;
  audio_dmc_level = output_level & 127;
  audio_dmc_cycle_fraction =
      AUDIO_DMC_PREROLL_CPU_CYCLES - AUDIO_DMC_PREROLL_CYCLES;
}

void audio_dmc_event(unsigned long long cpu_cycle, unsigned output_level) {
  uint32_t head = audio_dmc_head;
  uint32_t index = head & (AUDIO_DMC_EVENT_COUNT - 1);
  if (head - audio_dmc_tail >= AUDIO_DMC_EVENT_COUNT)
#ifdef SMOLNES_AUDIO_DMC_DIAGNOSTICS
  {
    audio_dmc_tail++;
    audio_dmc_overflows++;
  }
#else
    audio_dmc_tail++;
#endif
  /* Buffers are generated 60 ms ahead of playback; map APU time into that
     already-queued audio timeline so DMC edges land within the right block. */
  audio_dmc_event_cycles[index] =
      (uint32_t)(cpu_cycle + AUDIO_DMC_PREROLL_CYCLES);
  audio_dmc_event_levels[index] = (unsigned char)(output_level & 127);
  audio_dmc_head = head + 1;
#ifdef SMOLNES_AUDIO_DMC_DIAGNOSTICS
  if (cpu_cycle >= 21590000 && cpu_cycle <= 21655000) {
    if (!audio_dmc_enqueue_file) {
      audio_dmc_enqueue_file = fopen("audio-dmc-enqueue.csv", "w");
      if (audio_dmc_enqueue_file)
        fputs("cpu_cycle,level,event_cycle,head,tail\n",
              audio_dmc_enqueue_file);
    }
    if (audio_dmc_enqueue_file)
      fprintf(audio_dmc_enqueue_file, "%llu,%u,%u,%u,%u\n",
              cpu_cycle, output_level, audio_dmc_event_cycles[index],
              audio_dmc_head, audio_dmc_tail);
  }
#endif
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
  audio_test_dmc_output = NULL;
  audio_test_last_dmc_peak = 0;
  audio_render_frames(output, frame_count);
}

int audio_test_dmc_peak(void) { return audio_test_last_dmc_peak; }

void audio_test_render_with_dmc(short *output, short *dmc_output,
                                unsigned frame_count) {
  audio_test_dmc_output = dmc_output;
  audio_test_last_dmc_peak = 0;
#ifdef SMOLNES_AUDIO_DMC_DIAGNOSTICS
  audio_dmc_processed = 0;
#endif
  audio_render_frames(output, frame_count);
  audio_test_dmc_output = NULL;
#ifdef SMOLNES_AUDIO_DMC_DIAGNOSTICS
  if (!audio_dmc_diag_file) {
    audio_dmc_diag_file = fopen("audio-dmc-render.csv", "w");
    if (audio_dmc_diag_file)
      fputs("block,frames,render_cycle,event_head,event_tail,pending,processed,overflows,dac_level\n",
            audio_dmc_diag_file);
  }
  if (audio_dmc_diag_file) {
    fprintf(audio_dmc_diag_file, "%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
            audio_dmc_render_blocks++, frame_count, audio_dmc_cycle,
            audio_dmc_head, audio_dmc_tail,
            audio_dmc_head - audio_dmc_tail, audio_dmc_processed,
            audio_dmc_overflows, audio_dmc_level);
    fflush(audio_dmc_diag_file);
  }
#endif
}
#endif
