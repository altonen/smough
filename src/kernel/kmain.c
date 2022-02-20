#include <drivers/gfx/vga.h>
#include <fs/multiboot2.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/irq.h>
#include <kernel/pic.h>
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

    kprint("hello, world");
}
