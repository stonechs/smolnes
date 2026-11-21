#include <stdio.h>
#include "../../source/audio_win32.h"

int main(void) {
  short prefill[480 * 2];
  short output[16 * 2];
  unsigned i;

  audio_test_reset();
  audio_dmc_reset(64);
  for (i = 0; i < 6; i++)
    audio_test_render(prefill, 480);

  /* The emulator calls apu_reset() after audio_start() has prefetched blocks. */
  audio_dmc_reset(64);
  audio_dmc_event(0, 127);
  audio_test_render(output, 16);
  if (output[0] < 16000 || output[0] > 16300 || output[1] != output[0]) {
    fprintf(stderr, "FAIL: DMC event was not mixed after audio prefill: %d\n",
            output[0]);
    return 1;
  }
  puts("PASS: queued DMC DAC event reaches rendered PCM after 60 ms prefill");
  return 0;
}
