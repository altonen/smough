#include <arch/amd64/asm.h>
#include <kernel/idt.h>
#include <kernel/kprint.h>
#include <kernel/util.h>
#include <stdint.h>

/* defined in arch/amd64/interrupts.S */
extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr128(); /* 0x80 */

static struct idt_ptr_t idt_ptr;
static struct idt_entry_t idt_table[IDT_TABLE_SIZE] __attribute__((aligned(4)));

struct idt_entry_t *IDT = idt_table;

void idt_set_gate(unsigned long offset, uint16_t select, uint8_t type, struct idt_entry_t *entry)
{
    entry->offset0_15  = offset & 0xffff;
    entry->offset16_31 = (offset >> 16) & 0xffff;
    entry->offset32_63 = (offset >> 32) & 0xffffffff;
    entry->select      = select;
    entry->ist         = 0;
    entry->type        = type;
    entry->reserved1   = 0;
    entry->reserved2   = 0;
}

void idt_init(void)
{
    static bool once;

    if (!once) {
        kmemset(idt_table, 0, IDT_ENTRY_SIZE * IDT_TABLE_SIZE);

        idt_set_gate((unsigned long)isr0,   0x08, 0x8e, &idt_table[0]);
        idt_set_gate((unsigned long)isr1,   0x08, 0x8e, &idt_table[1]);
        idt_set_gate((unsigned long)isr2,   0x08, 0x8e, &idt_table[2]);
        idt_set_gate((unsigned long)isr3,   0x08, 0x8e, &idt_table[3]);
        idt_set_gate((unsigned long)isr4,   0x08, 0x8e, &idt_table[4]);
        idt_set_gate((unsigned long)isr5,   0x08, 0x8e, &idt_table[5]);
        idt_set_gate((unsigned long)isr6,   0x08, 0x8e, &idt_table[6]);
        idt_set_gate((unsigned long)isr7,   0x08, 0x8e, &idt_table[7]);
        idt_set_gate((unsigned long)isr8,   0x08, 0x8e, &idt_table[8]);
        idt_set_gate((unsigned long)isr9,   0x08, 0x8e, &idt_table[9]);
        idt_set_gate((unsigned long)isr10,  0x08, 0x8e, &idt_table[10]);
        idt_set_gate((unsigned long)isr11,  0x08, 0x8e, &idt_table[11]);
        idt_set_gate((unsigned long)isr12,  0x08, 0x8e, &idt_table[12]);
        idt_set_gate((unsigned long)isr13,  0x08, 0x8e, &idt_table[13]);
        idt_set_gate((unsigned long)isr14,  0x08, 0x8e, &idt_table[14]);
        idt_set_gate((unsigned long)isr15,  0x08, 0x8e, &idt_table[15]);
        idt_set_gate((unsigned long)isr16,  0x08, 0x8e, &idt_table[16]);
        idt_set_gate((unsigned long)isr17,  0x08, 0x8e, &idt_table[17]);
        idt_set_gate((unsigned long)isr18,  0x08, 0x8e, &idt_table[18]);
        idt_set_gate((unsigned long)isr19,  0x08, 0x8e, &idt_table[19]);
        idt_set_gate((unsigned long)isr20,  0x08, 0x8e, &idt_table[20]);
        idt_set_gate((unsigned long)isr128, 0x08, 0xee, &idt_table[128]);

        idt_ptr.limit = IDT_ENTRY_SIZE * 256 - 1;
        idt_ptr.base  = (unsigned long)idt_table;
        once          = 1;
    }

    idt_flush(&idt_ptr);
}
