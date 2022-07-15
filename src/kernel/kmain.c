#include <arch/amd64/mmu.h>
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
#include <kernel/percpu.h>
#include <kernel/pic.h>
#include <kernel/util.h>
#include <kernel/acpi/acpi.h>
#include <mm/mmu.h>

/* defined by the linker */
extern uint8_t _trampoline_start, _trampoline_end;
extern uint8_t _percpu_start, _percpu_end;
extern uint8_t _kernel_physical_end;

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

    /* initialize percpu areas for BSP and APs */
    uint64_t kernel_end = (uint64_t)&_kernel_physical_end;
    uint64_t percpu_start = ROUND_UP(kernel_end, PAGE_SIZE);
    size_t percpu_size = (uint64_t)&_percpu_end - (uint64_t)&_percpu_start;

    for (size_t i = 0; i < lapic_get_cpu_count(); ++i) {
        kmemcpy(
            (uint8_t *)&percpu_start + i * percpu_size,
            (uint8_t *)&_percpu_start,
            percpu_size
        );
    }

    /* initialize percpu state and GS base for BSP */
    percpu_init(0);

    kprint("hello, world\n");
}

void init_ap(void *arg)
{
    gdt_init();
    idt_init();

    for (;;);
}
