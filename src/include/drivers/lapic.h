#ifndef __LAPIC_H__
#define __LAPIC_H__

#include <stdint.h>

void lapic_initialize(void);
void lapic_register_dev(int cpu_id, int loapic_id);
void lapic_send_sipi(uint32_t cpu, uint32_t vec);
void lapic_send_ipi(uint32_t high, uint32_t low);
void lapic_send_init(uint32_t cpu);
void lapic_ack_interrupt(void);
uint32_t lapic_get_cpu_count(void);
uint32_t lapic_get_init_cpu_count(void);
int lapic_get_lapic_id(uint32_t cpu);
uint64_t lapic_get_base(void);

#endif /* __LAPIC_H__ */
