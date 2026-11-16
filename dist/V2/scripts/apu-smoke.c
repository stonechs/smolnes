#include <stdio.h>
#include <windows.h>
#include "../apu.h"
#include "../audio_win32.h"

int main(void) {
  unsigned long blocks;
  uint8_t status;
  if (!audio_start()) {
    fputs("FAIL: waveOut initialization failed\n", stderr);
    return 1;
  }

  apu_reset();
  if (apu_read_status() != 0) {
    fputs("FAIL: reset status was not muted\n", stderr);
    audio_stop();
    return 1;
  }

  /* Length counters and the four-step frame IRQ are cycle-clocked. */
  apu_write(0x4015, 0x0f);
  apu_write(0x4003, 0x18); /* length table index 3 => 2 half-frame ticks */
  apu_write(0x4007, 0x18);
  apu_write(0x400b, 0x18);
  apu_write(0x400f, 0x18);
  if (apu_read_status() != 0x0f) {
    fputs("FAIL: length counters did not load on channel writes\n", stderr);
    audio_stop();
    return 1;
  }
  apu_clock(29830);
  status = apu_read_status();
  if (status != 0x40 || apu_read_status() != 0) {
    fprintf(stderr, "FAIL: frame sequencer/length counters returned 0x%02x\n",
            status);
    audio_stop();
    return 1;
  }

  apu_reset();

  apu_write(0x4015, 0x0f);
  apu_write(0x4000, 0x0f);
  apu_write(0x4002, 0xfe);
  apu_write(0x4003, 0x00);
  apu_write(0x4004, 0x0c);
  apu_write(0x4006, 0xa7);
  apu_write(0x4007, 0x00);
  apu_write(0x4008, 0x7f);
  apu_write(0x400a, 0xfe);
  apu_write(0x400b, 0x00);
  apu_write(0x400c, 0x0f);
  apu_write(0x400e, 0x08);
  apu_write(0x400f, 0x00);
  status = apu_read_status();
  if (status != 0x0f) {
    fprintf(stderr, "FAIL: expected status 0x0f, got 0x%02x\n", status);
    audio_stop();
    return 1;
  }

  Sleep(2500);
  apu_write(0x4015, 0x05);
  status = apu_read_status();
  if (status != 0x05) {
    fprintf(stderr, "FAIL: expected status 0x05, got 0x%02x\n", status);
    audio_stop();
    return 1;
  }
  Sleep(2500);

  blocks = audio_completed_blocks();
  printf("status_after_reset=0x00\n");
  printf("status_enabled=0x0f\n");
  printf("status_after_disable=0x%02x\n", status);
  printf("blocks_completed=%lu\n", blocks);
  printf("underrun_events=%lu\n", audio_underruns());
  if (blocks < 400 || audio_underruns() != 0) {
    fputs("FAIL: audio output did not sustain the register test\n", stderr);
    audio_stop();
    return 1;
  }
  audio_stop();
  puts("PASS: APU register state drove four oscillators");
  return 0;
}
