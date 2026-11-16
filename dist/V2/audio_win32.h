#ifndef SMOLNES_AUDIO_WIN32_H
#define SMOLNES_AUDIO_WIN32_H

enum {
  AUDIO_VOICE_SQUARE = 0,
  AUDIO_VOICE_TRIANGLE = 1,
  AUDIO_VOICE_NOISE = 2,
  AUDIO_VOICE_COUNT = 4
};

int audio_start(void);
void audio_set_voice(unsigned voice, int waveform, double frequency,
                     double gain);
void audio_set_pulse_duty(unsigned voice, unsigned duty);
void audio_set_noise_mode(unsigned mode);
void audio_stop(void);
unsigned long audio_completed_blocks(void);
unsigned long audio_underruns(void);

#ifdef AUDIO_TESTING
void audio_test_reset(void);
void audio_test_render(short *output, unsigned frame_count);
#endif

#endif
