#ifndef SMOLNES_AUDIO_WIN32_H
#define SMOLNES_AUDIO_WIN32_H

enum {
  AUDIO_VOICE_SQUARE = 0,
  AUDIO_VOICE_TRIANGLE = 1,
  AUDIO_VOICE_NOISE = 2,
  AUDIO_VOICE_COUNT = 4
};

int audio_start(void *callback_window);
void audio_handle_done(void *completed_header);
void audio_set_voice(unsigned voice, int waveform, double frequency,
                     double gain);
void audio_set_pulse_duty(unsigned voice, unsigned duty);
void audio_set_noise_mode(unsigned mode);
void audio_dmc_reset(unsigned output_level);
void audio_dmc_event(unsigned long long cpu_cycle, unsigned output_level);
void audio_stop(void);

#ifdef AUDIO_TESTING
void audio_test_reset(void);
void audio_test_render(short *output, unsigned frame_count);
int audio_test_dmc_peak(void);
void audio_test_render_with_dmc(short *output, short *dmc_output,
                                unsigned frame_count);
#endif

#endif
