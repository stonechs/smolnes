#include <stdint.h>
#include <stdio.h>
#include "../../source/apu.h"

static unsigned read_count;
static uint16_t last_address;
static unsigned last_level = 64;
static unsigned event_count;
static unsigned event_levels[32];

static uint8_t test_read(uint16_t address) {
  read_count++;
  last_address = address;
  return 0xa5; /* LSB-first sequence: 1,0,1,0,0,1,0,1. */
}

void audio_set_voice(unsigned voice, int waveform, double frequency,
                     double gain) {
  (void)voice; (void)waveform; (void)frequency; (void)gain;
}
void audio_set_pulse_duty(unsigned voice, unsigned duty) {
  (void)voice; (void)duty;
}
void audio_set_noise_mode(unsigned mode) { (void)mode; }
void audio_dmc_reset(unsigned output_level) { last_level = output_level; }
void audio_dmc_event(unsigned long long cycle, unsigned output_level) {
  (void)cycle;
  last_level = output_level;
  if (event_count < sizeof(event_levels) / sizeof(event_levels[0]))
    event_levels[event_count++] = output_level;
}

static int fail(const char *message) {
  fprintf(stderr, "FAIL: %s\n", message);
  return 1;
}

int main(void) {
  unsigned stalls;
  apu_set_memory_reader(test_read);
  apu_reset();
  apu_write(0x4010, 0x80); /* IRQ enabled, rate index 0 (428 CPU cycles). */
  apu_write(0x4011, 64);
  apu_write(0x4012, 0x00);
  apu_write(0x4013, 0x00); /* one byte */
  apu_write(0x4015, 0x10);
  stalls = apu_clock(1);
  if (stalls != 4 || read_count != 1 || last_address != 0xc000)
    return fail("sample fetch address or four-cycle DMA stall");
  if (!(apu_read_status() & 0x80) || !apu_irq_pending())
    return fail("DMC IRQ status/pending");
  event_count = 0;
  /* The first buffered byte is loaded only at an eight-clock boundary. */
  (void)apu_clock(17 * 428);
  if (event_count != 16 || event_levels[8] != 66 ||
      event_levels[9] != 64 || event_levels[10] != 66 ||
      event_levels[11] != 64 || event_levels[12] != 62 ||
      event_levels[13] != 64 || event_levels[14] != 62 ||
      event_levels[15] != 64 || last_level != 64) {
    unsigned i;
    fprintf(stderr, "events=%u last=%u levels:", event_count, last_level);
    for (i = 0; i < event_count; i++)
      fprintf(stderr, " %u", event_levels[i]);
    fputc('\n', stderr);
    return fail("DMC byte was not decoded LSB-first at the byte boundary");
  }
  apu_write(0x4015, 0);
  if (apu_irq_pending() || (apu_read_status() & 0x10))
    return fail("DMC disable did not clear IRQ and active status");

  apu_reset();
  read_count = 0;
  apu_write(0x4010, 0x40); /* looping */
  apu_write(0x4012, 0xff);
  apu_write(0x4013, 0x00);
  apu_write(0x4015, 0x10);
  (void)apu_clock(1);
  if (read_count != 1 || last_address != 0xffc0 ||
      !(apu_read_status() & 0x10))
    return fail("looping sample did not reload address/length");
  puts("PASS: DMC fetch, DMA stall, IRQ, DAC output, disable, and loop");
  return 0;
}
