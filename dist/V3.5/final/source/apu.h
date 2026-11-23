#ifndef SMOLNES_APU_H
#define SMOLNES_APU_H

#include <stdint.h>

typedef uint8_t (*ApuMemoryReader)(uint16_t address);

void apu_set_memory_reader(ApuMemoryReader reader);
void apu_reset(void);
void apu_write(uint16_t address, uint8_t value);
uint8_t apu_read_status(void);
unsigned apu_clock(unsigned cpu_cycles);
int apu_irq_pending(void);
#ifdef SMOLNES_AUDIO_DMC_DIAGNOSTICS
uint64_t apu_diagnostic_time(void);
#endif

#endif
