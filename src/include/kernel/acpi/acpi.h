#ifndef __ACPI_H__
#define __ACPI_H__

#include <stdint.h>

/* initialize ACPI */
int acpi_init(void);

/* return address of local apic discovered from madt on success */
uint64_t acpi_get_local_apic_addr(void);

#endif /* __ACPI_H__ */
