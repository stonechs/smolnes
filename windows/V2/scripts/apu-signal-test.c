#include <stdio.h>
#include <stdlib.h>
#include "../apu.h"
#include "../audio_win32.h"

#define SAMPLE_RATE 48000

int main(void) {
  short *pcm = (short *)calloc(SAMPLE_RATE * 2, sizeof(short));
  unsigned i, crossings = 0;
  int has_positive = 0, has_negative = 0;
  if (!pcm)
    return 2;

  audio_test_reset();
  apu_reset();
  apu_write(0x4015, 0x01);
  apu_write(0x4000, 0xb5); /* 50% duty, constant volume 5 */
  apu_write(0x4001, 0x08); /* Pulse 1 negate, shift zero; target is -1 */
  apu_write(0x4002, 0xf1);
  apu_write(0x4003, 0x09); /* Timer 0x1f1, load a nonzero length */
  if ((apu_read_status() & 1) == 0) {
    fputs("FAIL: pulse length did not load\n", stderr);
    free(pcm);
    return 1;
  }

  audio_test_render(pcm, SAMPLE_RATE);
  for (i = 0; i < SAMPLE_RATE; i++) {
    short sample = pcm[i * 2];
    if (sample > 200)
      has_positive = 1;
    if (sample < -200)
      has_negative = 1;
    if (i && pcm[(i - 1) * 2] <= 0 && sample > 0)
      crossings++;
  }
  printf("pulse1_negative_sweep_zero_shift_crossings=%u\n", crossings);
  free(pcm);
  if (crossings < 220 || crossings > 228 || !has_positive || !has_negative) {
    fputs("FAIL: negative sweep target incorrectly muted pulse 1\n", stderr);
    return 1;
  }
  puts("PASS: negative sweep target remains audible");
  return 0;
}
