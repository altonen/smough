#include <drivers/gfx/vga.h>
#include <drivers/ioapic.h>
#include <fs/multiboot2.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/irq.h>
#include <kernel/pic.h>
#include <kernel/acpi/acpi.h>
#include <mm/mmu.h>

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

    kprint("hello, world");
}
