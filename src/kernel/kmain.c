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
#include <kernel/acpi/acpi.h>
#include <mm/mmu.h>

/* defined by the linker */
extern uint8_t _trampoline_start;
extern uint8_t _trampoline_end;

void kmain(void *arg)
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

    kprint("hello, world\n");
    kprint("trampoline size: 0x%x %u\n",
        (size_t)&_trampoline_end - (size_t)&_trampoline_start,
        (size_t)&_trampoline_end - (size_t)&_trampoline_start
    );
}
