#include <arch/amd64/cpu.h>
#include <drivers/gfx/vga.h>
#include <drivers/gfx/vbe.h>
#include <drivers/ioapic.h>
#include <drivers/lapic.h>
#include <drivers/device.h>
#include <drivers/bus/pci.h>
#include <fs/multiboot2.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/irq.h>
#include <kernel/pic.h>
#include <kernel/util.h>
#include <kernel/acpi/acpi.h>
#include <mm/mmu.h>

/* defined by the linker */
extern uint8_t _trampoline_start;
extern uint8_t _trampoline_end;

volatile unsigned ap_initialized = 0;

void init_bsp(void *arg)
{
    /* GDT, IDT, and PIC */
    gdt_init(); idt_init(); pic_init();

    /* vga for kprint */
    vga_init();

    /* initialize memory manager and all memory allocators */
    mm_init(arg);

	/* parse elf symbol table for kassert() */
    multiboot2_parse_elf(arg);

    /* initialize ACPI and I/O APIC */
    acpi_init();
    ioapic_initialize_all();
    lapic_initialize();

    dev_init();
    pci_init();
    vbe_init();

    /* copy SMP trampoline to 0x55000 */
    size_t trmp_size = (size_t)&_trampoline_end - (size_t)&_trampoline_start;
    kmemcpy((uint8_t *)0x55000, &_trampoline_start, trmp_size);

    for (size_t i = 1; i < lapic_get_cpu_count(); ++i) {
        lapic_send_init(i);
        for (volatile unsigned k = 0; k < 50000000; ++k)
            ;
        lapic_send_sipi(i, 0x55);

        while (ap_initialized == 0) {
            cpu_relax();
        }

        kprint("ap_initialized %u\n", ap_initialized);
    }

    kprint("all %u CPUs initialized\n", ap_initialized);

    for (;;);
}

void init_ap(void *arg)
{
    gdt_init();
    idt_init();

    ap_initialized++;

    for (;;);
}
