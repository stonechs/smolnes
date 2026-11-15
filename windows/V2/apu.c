#include <string.h>
#ifdef SMOLNES_APU_TRACE
#include <stdio.h>
#endif
#include "apu.h"
#include "audio_win32.h"

enum { PULSE1, PULSE2, TRIANGLE, NOISE, CHANNEL_COUNT };

typedef struct {
  unsigned timer;
  unsigned length;
  uint8_t control;
  uint8_t envelope;
  uint8_t envelope_divider;
  uint8_t envelope_start;
  uint8_t sweep;
  uint8_t sweep_divider;
  uint8_t sweep_reload;
  uint8_t linear;
  uint8_t linear_reload;
} ApuChannel;

static ApuChannel channels[CHANNEL_COUNT];
static uint8_t enabled_channels;
static uint8_t frame_mode;
static uint8_t frame_step;
static uint8_t frame_irq;
static uint8_t frame_irq_inhibit;
static uint64_t apu_time;
static uint64_t frame_base;

#ifdef SMOLNES_APU_TRACE
static FILE *apu_trace_file;
static unsigned apu_trace_count;

static void trace_apu_write(uint16_t address, uint8_t value) {
  if (!apu_trace_file) {
    apu_trace_file = fopen("apu-register-trace.csv", "w");
    if (apu_trace_file)
      fputs("cpu_cycle,address,value\n", apu_trace_file);
  }
  if (apu_trace_file && apu_trace_count < 20000) {
    fprintf(apu_trace_file, "%llu,0x%04X,0x%02X\n",
            (unsigned long long)apu_time, address, value);
    fflush(apu_trace_file);
    apu_trace_count++;
  }
}
#endif

static const unsigned noise_periods[16] = {
    4, 8, 16, 32, 64, 96, 128, 160,
    202, 254, 380, 508, 762, 1016, 2034, 4068};

static const uint8_t length_table[32] = {
    10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30};

static const unsigned frame_times[5] = {7457, 14913, 22371, 29829, 37281};

static int pulse_target(unsigned channel) {
  ApuChannel *s = &channels[channel];
  unsigned shift = s->sweep & 7;
  int delta = (int)(s->timer >> shift);
  if (s->sweep & 8)
    return (int)s->timer - delta - (channel == PULSE1 ? 1 : 0);
  return (int)s->timer + delta;
}

static unsigned channel_volume(unsigned channel) {
  ApuChannel *s = &channels[channel];
  if (s->control & 16)
    return s->control & 15;
  return s->envelope;
}

static void render_channels(void) {
  unsigned ch;
  for (ch = 0; ch < CHANNEL_COUNT; ch++) {
    ApuChannel *s = &channels[ch];
    double gain = 0.0;
    double frequency = 0.0;
    int waveform = ch == TRIANGLE ? AUDIO_VOICE_TRIANGLE
                   : ch == NOISE ? AUDIO_VOICE_NOISE
                                 : AUDIO_VOICE_SQUARE;

    if (ch < 2) {
      int target = pulse_target(ch);
      if (s->length && s->timer >= 8 && target <= 0x7ff)
        gain = channel_volume(ch) / 15.0 * 0.08;
      frequency = 1789773.0 / (16.0 * (s->timer + 1.0));
      audio_set_pulse_duty(ch, s->control >> 6);
    } else if (ch == TRIANGLE) {
      if (s->length && s->linear && s->timer >= 2)
        gain = 0.08;
      frequency = 1789773.0 / (32.0 * (s->timer + 1.0));
    } else {
      if (s->length)
        gain = channel_volume(ch) / 15.0 * 0.035;
      frequency = 1789773.0 / noise_periods[s->timer & 15];
      audio_set_noise_mode((s->timer & 0x80) != 0);
    }

    if (!(enabled_channels & (1u << ch)))
      gain = 0.0;
    audio_set_voice(ch, waveform, frequency, gain);
  }
}

static void clock_quarter_frame(void) {
  unsigned ch;
  for (ch = PULSE1; ch <= NOISE; ch++) {
    ApuChannel *s = &channels[ch];
    unsigned period;
    if (ch == TRIANGLE)
      continue;
    period = s->control & 15;
    if (s->envelope_start) {
      s->envelope_start = 0;
      s->envelope = 15;
      s->envelope_divider = (uint8_t)period;
    } else if (s->envelope_divider) {
      s->envelope_divider--;
    } else {
      s->envelope_divider = (uint8_t)period;
      if (s->envelope)
        s->envelope--;
      else if (s->control & 32)
        s->envelope = 15;
    }
  }

  {
    ApuChannel *s = &channels[TRIANGLE];
    if (s->linear_reload)
      s->linear = s->control & 0x7f;
    else if (s->linear)
      s->linear--;
    if (!(s->control & 0x80))
      s->linear_reload = 0;
  }
}

static void clock_half_frame(void) {
  unsigned ch;
  for (ch = PULSE1; ch <= NOISE; ch++) {
    ApuChannel *s = &channels[ch];
    unsigned halt = ch == TRIANGLE ? 0x80 : 0x20;
    if (s->length && !(s->control & halt))
      s->length--;

    if (ch < TRIANGLE) {
      int target = pulse_target(ch);
      if (!s->sweep_divider && (s->sweep & 0x80) && (s->sweep & 7) &&
          s->timer >= 8 && target <= 0x7ff)
        s->timer = (unsigned)target;
      if (!s->sweep_divider || s->sweep_reload) {
        s->sweep_divider = (s->sweep >> 4) & 7;
        s->sweep_reload = 0;
      } else {
        s->sweep_divider--;
      }
    }
  }
}

static void clock_frame_event(void) {
  unsigned step = frame_step;
  if (step != 3 || !frame_mode)
    clock_quarter_frame();
  if (step == 1 || step == (frame_mode ? 4u : 3u))
    clock_half_frame();
  if (!frame_mode && step == 3 && !frame_irq_inhibit)
    frame_irq = 1;

  frame_step++;
  if (frame_step == (frame_mode ? 5 : 4)) {
    frame_base += frame_mode ? 37282 : 29830;
    frame_step = 0;
  }
  render_channels();
}

void apu_clock(unsigned cpu_cycles) {
  uint64_t end = apu_time + cpu_cycles;
  while (apu_time < end) {
    uint64_t event_time = frame_base + frame_times[frame_step];
    if (event_time > end) {
      apu_time = end;
      break;
    }
    apu_time = event_time;
    clock_frame_event();
  }
}

void apu_reset(void) {
  memset(channels, 0, sizeof(channels));
  enabled_channels = frame_mode = frame_step = frame_irq = 0;
  frame_irq_inhibit = 0;
  apu_time = frame_base = 0;
  render_channels();
}

uint8_t apu_read_status(void) {
  uint8_t status = frame_irq ? 0x40 : 0;
  unsigned ch;
  for (ch = 0; ch < CHANNEL_COUNT; ch++)
    if (channels[ch].length)
      status |= (uint8_t)(1u << ch);
  frame_irq = 0;
  render_channels();
  return status;
}

int apu_irq_pending(void) { return frame_irq != 0; }

void apu_write(uint16_t address, uint8_t value) {
  unsigned channel;
  unsigned reg;
  ApuChannel *s;

#ifdef SMOLNES_APU_TRACE
  trace_apu_write(address, value);
#endif

  if (address >= 0x4000 && address <= 0x4007) {
    channel = (address - 0x4000) >> 2;
    reg = address & 3;
    s = &channels[channel];
    if (reg == 0) {
      s->control = value;
    } else if (reg == 1) {
      s->sweep = value;
      s->sweep_reload = 1;
    } else if (reg == 2) {
      s->timer = (s->timer & 0x700) | value;
    } else {
      s->timer = (s->timer & 0xff) | ((unsigned)(value & 7) << 8);
      if (enabled_channels & (1u << channel))
        s->length = length_table[value >> 3];
      s->envelope_start = 1;
    }
    render_channels();
    return;
  }

  switch (address) {
  case 0x4008:
    channels[TRIANGLE].control = value;
    break;
  case 0x400a:
    channels[TRIANGLE].timer =
        (channels[TRIANGLE].timer & 0x700) | value;
    break;
  case 0x400b:
    s = &channels[TRIANGLE];
    s->timer = (s->timer & 0xff) | ((unsigned)(value & 7) << 8);
    if (enabled_channels & (1u << TRIANGLE))
      s->length = length_table[value >> 3];
    s->linear_reload = 1;
    break;
  case 0x400c:
    channels[NOISE].control = value;
    break;
  case 0x400e:
    channels[NOISE].timer = value;
    break;
  case 0x400f:
    s = &channels[NOISE];
    if (enabled_channels & (1u << NOISE))
      s->length = length_table[value >> 3];
    s->envelope_start = 1;
    break;
  case 0x4015:
    enabled_channels = value & 15;
    for (channel = 0; channel < CHANNEL_COUNT; channel++)
      if (!(enabled_channels & (1u << channel)))
        channels[channel].length = 0;
    break;
  case 0x4017:
    frame_mode = value >> 7;
    frame_irq_inhibit = (value >> 6) & 1;
    if (frame_irq_inhibit)
      frame_irq = 0;
    frame_base = apu_time;
    frame_step = 0;
    if (frame_mode) {
      clock_quarter_frame();
      clock_half_frame();
    }
    break;
  default:
    /* DMC registers $4010-$4013 are intentionally not implemented. */
    return;
  }
  render_channels();
}
