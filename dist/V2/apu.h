#ifndef SMOLNES_APU_H
#define SMOLNES_APU_H

#include <stdint.h>

void apu_reset(void);
void apu_write(uint16_t address, uint8_t value);
uint8_t apu_read_status(void);
void apu_clock(unsigned cpu_cycles);
int apu_irq_pending(void);

#endif
