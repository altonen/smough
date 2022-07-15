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

    lapic_send_init(1);
    for (volatile unsigned k = 0; k < 5000000; ++k)
        ;
    lapic_send_sipi(1, 0x55);

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
