#ifndef __IO_APIC_H__
#define __IO_APIC_H__

#include <stdint.h>

void ioapic_initialize_all(void);
void ioapic_register_dev(uint8_t ioapic_id, uint32_t ioapic_addr, uint32_t intr_base);
void ioapic_enable_irq(unsigned cpu, unsigned irq);
void ioapic_assign_ioint(uint8_t bus_id, uint8_t bus_irq, uint8_t ioapic_id, uint8_t ioapic_irq);
unsigned long ioapic_get_base(void);

#endif /* __IO_APIC_H__ */
