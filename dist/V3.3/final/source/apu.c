#include <string.h>
#if defined(SMOLNES_APU_TRACE) || defined(SMOLNES_DMC_OUTPUT_TRACE)
#include <stdio.h>
#endif
#include "apu.h"
#include "audio_win32.h"

enum { PULSE1, PULSE2, TRIANGLE, NOISE, CHANNEL_COUNT };

enum { DMC_LOOP = 0x40, DMC_IRQ_ENABLE = 0x80 };

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
static ApuMemoryReader dmc_memory_reader;

static struct {
  uint16_t sample_address;
  uint16_t sample_length;
  uint16_t current_address;
  uint16_t bytes_remaining;
  uint16_t timer_period;
  uint16_t timer_counter;
  uint8_t control;
  uint8_t output_level;
  uint8_t sample_buffer;
  uint8_t sample_buffer_empty;
  uint8_t shift_register;
  uint8_t bits_remaining;
  uint8_t silence;
  uint8_t irq;
  uint8_t fetch_pending;
} dmc;


#ifdef SMOLNES_APU_TRACE
static FILE *apu_trace_file;
static FILE *apu_dmc_trace_file;
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

static void trace_dmc_fetch(uint16_t address, uint8_t value) {
  if (!apu_dmc_trace_file) {
    apu_dmc_trace_file = fopen("apu-dmc-fetch.csv", "w");
    if (apu_dmc_trace_file)
      fputs("cpu_cycle,address,value\n", apu_dmc_trace_file);
  }
  if (apu_dmc_trace_file)
    fprintf(apu_dmc_trace_file, "%llu,0x%04X,0x%02X\n",
            (unsigned long long)apu_time, address, value);
}
#endif

#ifdef SMOLNES_DMC_OUTPUT_TRACE
static FILE *dmc_output_trace_file;
static void trace_dmc_output(void) {
  if (apu_time < 21590000 || apu_time > 21655000)
    return;
  if (!dmc_output_trace_file) {
    dmc_output_trace_file = fopen("dmc-output-trace.csv", "w");
    if (dmc_output_trace_file)
      fputs("cpu_cycle,level,shift,bits_remaining,silence,buffer_empty,bytes_remaining,fetch_pending\n",
            dmc_output_trace_file);
  }
  if (dmc_output_trace_file)
    fprintf(dmc_output_trace_file, "%llu,%u,%02X,%u,%u,%u,%u,%u\n",
            (unsigned long long)apu_time, dmc.output_level,
            dmc.shift_register, dmc.bits_remaining, dmc.silence,
            dmc.sample_buffer_empty, dmc.bytes_remaining, dmc.fetch_pending);
}
#endif

static const unsigned noise_periods[16] = {
    4, 8, 16, 32, 64, 96, 128, 160,
    202, 254, 380, 508, 762, 1016, 2034, 4068};

static const uint8_t length_table[32] = {
    10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30};

static const unsigned frame_times[5] = {7457, 14913, 22371, 29829, 37281};
static const uint16_t dmc_periods[16] = {
    428, 380, 340, 320, 286, 254, 226, 214,
    190, 160, 142, 128, 106, 84, 72, 54};

#ifdef SMOLNES_REVERSE_CONTRA_DPCM_BITS
static uint8_t reverse_dpcm_byte(uint8_t value) {
  value = (uint8_t)((value >> 4) | (value << 4));
  value = (uint8_t)(((value & 0xcc) >> 2) | ((value & 0x33) << 2));
  return (uint8_t)(((value & 0xaa) >> 1) | ((value & 0x55) << 1));
}
#endif

static int dmc_active(void) {
  return dmc.bytes_remaining || dmc.fetch_pending ||
         !dmc.sample_buffer_empty || (!dmc.silence && dmc.bits_remaining);
}

static void dmc_restart(void) {
  dmc.current_address = dmc.sample_address;
  dmc.bytes_remaining = dmc.sample_length;
  if (dmc.sample_buffer_empty)
    dmc.fetch_pending = dmc.bytes_remaining != 0;
}

static void dmc_fetch_sample(void) {
  dmc.fetch_pending = 0;
  if (dmc.sample_buffer_empty && dmc.bytes_remaining) {
    uint16_t address = dmc.current_address;
    uint8_t value = dmc_memory_reader
                        ? dmc_memory_reader(dmc.current_address)
                        : 0;
#ifdef SMOLNES_APU_TRACE
    trace_dmc_fetch(address, value);
#endif
#ifdef SMOLNES_REVERSE_CONTRA_DPCM_BITS
    value = reverse_dpcm_byte(value);
#endif
    dmc.sample_buffer = value;
    dmc.sample_buffer_empty = 0;
    dmc.current_address = dmc.current_address == 0xffff
                              ? 0x8000
                              : (uint16_t)(dmc.current_address + 1);
    dmc.bytes_remaining--;
    if (!dmc.bytes_remaining) {
      if (dmc.control & DMC_LOOP) {
        dmc.current_address = dmc.sample_address;
        dmc.bytes_remaining = dmc.sample_length;
      } else if (dmc.control & DMC_IRQ_ENABLE) {
        dmc.irq = 1;
      }
    }
  }
}

static void dmc_clock_output(void) {
  if (!dmc.silence) {
    if (dmc.shift_register & 1) {
      if (dmc.output_level <= 125)
        dmc.output_level += 2;
    } else if (dmc.output_level >= 2) {
      dmc.output_level -= 2;
    }
    dmc.shift_register >>= 1;
  }

  /* The bit counter clocks even while silent. A byte is loaded only after
     the current eight-bit output cycle ends, not before its first bit. */
  if (dmc.bits_remaining)
    dmc.bits_remaining--;
  if (!dmc.bits_remaining) {
    dmc.bits_remaining = 8;
    if (dmc.sample_buffer_empty) {
      dmc.silence = 1;
    } else {
      dmc.shift_register = dmc.sample_buffer;
      dmc.sample_buffer_empty = 1;
      dmc.silence = 0;
      if (dmc.bytes_remaining)
        dmc.fetch_pending = 1;
    }
  }
#ifdef SMOLNES_DMC_OUTPUT_TRACE
  trace_dmc_output();
#endif
  audio_dmc_event(apu_time, dmc.output_level);
}

static void dmc_write_status(unsigned enabled) {
  dmc.irq = 0;
  if (!(enabled & 0x10)) {
    dmc.bytes_remaining = 0;
    dmc.fetch_pending = 0;
  } else if (!dmc.bytes_remaining) {
    dmc_restart();
  }
}

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

unsigned apu_clock(unsigned cpu_cycles) {
  uint64_t end = apu_time + cpu_cycles;
  unsigned dma_stalls = 0;

  while (apu_time < end) {
    uint64_t frame_event = frame_base + frame_times[frame_step];
    uint64_t delta = end - apu_time;
    int active = dmc_active();

    if (frame_event > apu_time && frame_event - apu_time < delta)
      delta = frame_event - apu_time;
    if (active && dmc.timer_counter < delta)
      delta = dmc.timer_counter;
    apu_time += delta;
    if (active) {
      dmc.timer_counter = (uint16_t)(dmc.timer_counter - delta);
    }

    if (apu_time == frame_event)
      clock_frame_event();

    if (active && !dmc.timer_counter) {
      dmc.timer_counter = dmc.timer_period;
      dmc_clock_output();
    }

    if (dmc.fetch_pending) {
      dmc_fetch_sample();
      end += 4;
      dma_stalls += 4;
    }
  }
  return dma_stalls;
}

void apu_set_memory_reader(ApuMemoryReader reader) {
  dmc_memory_reader = reader;
}

void apu_reset(void) {
  memset(channels, 0, sizeof(channels));
  memset(&dmc, 0, sizeof(dmc));
  dmc.sample_buffer_empty = 1;
  dmc.silence = 1;
  dmc.bits_remaining = 8;
  dmc.timer_period = dmc_periods[0];
  audio_dmc_reset(64);
  enabled_channels = frame_mode = frame_step = frame_irq = 0;
  frame_irq_inhibit = 0;
  apu_time = frame_base = 0;
  render_channels();
}

uint8_t apu_read_status(void) {
  uint8_t status = (frame_irq ? 0x40 : 0) | (dmc.irq ? 0x80 : 0) |
                   (dmc.bytes_remaining ? 0x10 : 0);
  unsigned ch;
  for (ch = 0; ch < CHANNEL_COUNT; ch++)
    if (channels[ch].length)
      status |= (uint8_t)(1u << ch);
  frame_irq = 0;
  render_channels();
  return status;
}

int apu_irq_pending(void) { return frame_irq != 0 || dmc.irq != 0; }

#ifdef SMOLNES_AUDIO_DMC_DIAGNOSTICS
uint64_t apu_diagnostic_time(void) { return apu_time; }
#endif

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

  if (address >= 0x4010 && address <= 0x4013) {
    switch (address) {
    case 0x4010:
      dmc.control = value;
      dmc.timer_period = dmc_periods[value & 15];
      if (!(value & DMC_IRQ_ENABLE))
        dmc.irq = 0;
      if (!dmc.timer_counter || dmc_active())
        dmc.timer_counter = dmc.timer_period;
      break;
    case 0x4011:
      dmc.output_level = value & 0x7f;
      audio_dmc_event(apu_time, dmc.output_level);
      break;
    case 0x4012:
      dmc.sample_address = (uint16_t)(0xc000 + ((unsigned)value << 6));
      break;
    case 0x4013:
      dmc.sample_length = (uint16_t)(((unsigned)value << 4) | 1);
      break;
    }
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
    dmc_write_status(value);
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
    return;
  }
  render_channels();
}
