#ifndef __PS2_H__
#define __PS2_H__

#include <arch/amd64/cpu.h>

// initialize ps2
int ps2_init(void);

// read next character from ps2
unsigned char ps2_read_next(void);

// interrupt handler for ps2 driver
void ps2_isr_handler(cpu_state_t *cpu);

#endif /* __PS2_H__ */
