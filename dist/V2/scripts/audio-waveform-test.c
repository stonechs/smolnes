#include <stdio.h>
#include <stdlib.h>
#include "../audio_win32.h"

#define SAMPLE_RATE 48000

static unsigned positive_crossings(const short *samples, unsigned frames) {
  unsigned i, crossings = 0;
  for (i = 1; i < frames; i++)
    if (samples[(i - 1) * 2] <= 0 && samples[i * 2] > 0)
      crossings++;
  return crossings;
}

static unsigned positive_samples(const short *samples, unsigned frames) {
  unsigned i, count = 0;
  for (i = 0; i < frames; i++)
    if (samples[i * 2] > 1000)
      count++;
  return count;
}

static int test_tone(int waveform, double frequency, const char *name) {
  short *pcm = (short *)calloc(SAMPLE_RATE * 2, sizeof(short));
  unsigned crossings;
  unsigned i;
  int has_positive = 0, has_negative = 0;
  if (!pcm)
    return 0;
  audio_test_reset();
  audio_set_voice(0, waveform, frequency, 0.4);
  audio_test_render(pcm, SAMPLE_RATE);
  crossings = positive_crossings(pcm, SAMPLE_RATE);
  for (i = 0; i < SAMPLE_RATE; i++) {
    if (pcm[i * 2] > 1000)
      has_positive = 1;
    if (pcm[i * 2] < -1000)
      has_negative = 1;
  }
  printf("%s_positive_crossings=%u\n", name, crossings);
  free(pcm);
  return crossings >= 439 && crossings <= 441 && has_positive && has_negative;
}

int main(void) {
  short *pcm, *noise_long, *noise_short;
  unsigned i, clipped = 0, duty12, duty50;
  unsigned noise_mode_differences = 0;
  int smooth;
  int passed = 1;

  passed &= test_tone(AUDIO_VOICE_SQUARE, 440.0, "square_440hz");
  passed &= test_tone(AUDIO_VOICE_TRIANGLE, 440.0, "triangle_440hz");

  pcm = (short *)calloc(4800 * 2, sizeof(short));
  if (!pcm)
    return 2;
  audio_test_reset();
  audio_set_voice(0, AUDIO_VOICE_SQUARE, 1000.0, 0.4);
  audio_set_pulse_duty(0, 0);
  audio_test_render(pcm, 4800);
  duty12 = positive_samples(pcm, 4800);
  audio_test_reset();
  audio_set_voice(0, AUDIO_VOICE_SQUARE, 1000.0, 0.4);
  audio_set_pulse_duty(0, 2);
  audio_test_render(pcm, 4800);
  duty50 = positive_samples(pcm, 4800);
  printf("pulse_positive_samples_duty12=%u\n", duty12);
  printf("pulse_positive_samples_duty50=%u\n", duty50);
  passed &= duty12 < duty50 / 2;
  free(pcm);

  pcm = (short *)calloc(8192 * 2, sizeof(short));
  if (!pcm)
    return 2;
  audio_test_reset();
  audio_set_voice(0, AUDIO_VOICE_NOISE, 900.0, 0.4);
  audio_test_render(pcm, 8192);
  {
    int previous_sign = 0;
    int has_positive = 0, has_negative = 0;
    for (i = 0; i < 8192; i++) {
      int sample = pcm[i * 2];
      int sign = sample > 1000 ? 1 : sample < -1000 ? -1 : 0;
      if (sign > 0)
        has_positive = 1;
      else if (sign < 0)
        has_negative = 1;
      if (sign && previous_sign && sign != previous_sign)
        clipped++;
      if (sign)
        previous_sign = sign;
    }
    printf("noise_sign_changes=%u\n", clipped);
    passed &= clipped > 4 && has_positive && has_negative;
  }

  audio_test_reset();
  audio_set_voice(0, AUDIO_VOICE_SQUARE, 440.0, 0.5);
  audio_set_voice(1, AUDIO_VOICE_SQUARE, 440.0, 0.5);
  audio_set_voice(2, AUDIO_VOICE_SQUARE, 440.0, 0.5);
  audio_set_voice(3, AUDIO_VOICE_SQUARE, 440.0, 0.0);
  audio_set_pulse_duty(0, 2);
  audio_set_pulse_duty(1, 2);
  audio_test_render(pcm, 1024);
  clipped = 0;
  for (i = 0; i < 1024; i++)
    if (pcm[i * 2] == 32767 || pcm[i * 2] == -32768)
      clipped++;
  printf("saturated_samples=%u\n", clipped);
  passed &= clipped > 40;

  noise_long = (short *)calloc(4096 * 2, sizeof(short));
  noise_short = (short *)calloc(4096 * 2, sizeof(short));
  if (!noise_long || !noise_short)
    return 2;
  audio_test_reset();
  audio_set_voice(3, AUDIO_VOICE_NOISE, 12000.0, 0.4);
  audio_set_noise_mode(0);
  audio_test_render(noise_long, 4096);
  audio_test_reset();
  audio_set_voice(3, AUDIO_VOICE_NOISE, 12000.0, 0.4);
  audio_set_noise_mode(1);
  audio_test_render(noise_short, 4096);
  for (i = 0; i < 4096; i++)
    if (noise_long[i * 2] != noise_short[i * 2])
      noise_mode_differences++;
  printf("noise_mode_different_samples=%u\n", noise_mode_differences);
  passed &= noise_mode_differences > 100;
  free(noise_long);
  free(noise_short);

  audio_test_reset();
  audio_set_voice(0, AUDIO_VOICE_SQUARE, 440.0, 0.3);
  audio_test_render(pcm, 480);
  audio_set_voice(0, AUDIO_VOICE_SQUARE, 880.0, 0.3);
  audio_test_render(pcm, 1);
  smooth = abs((int)pcm[0] - (int)pcm[480 * 2 - 2]);
  printf("frequency_change_sample_step=%d\n", smooth);
  passed &= smooth < 1000;
  free(pcm);

  if (!passed) {
    fputs("FAIL: waveform frequency, noise, saturation or phase continuity check failed\n",
          stderr);
    return 1;
  }
  puts("PASS: software waveform generation and mix boundaries");
  return 0;
}
