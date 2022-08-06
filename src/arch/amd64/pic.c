#include <arch/amd64/cpu.h>
#include <kernel/io.h>
#include <kernel/idt.h>
#include <kernel/pic.h>
#include <kernel/kprint.h>
#include <kernel/kpanic.h>
#include <kernel/common.h>

extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();

static void *irq_routines[16] = { 0 };

void pic_install_handler(void (*handler)(struct cpu_state *cpu), int irq_num)
{
    if (irq_num > 15 || irq_num < 0 || !handler) {
        kdebug("Invalid IRQ handler or IRQ Number");
        return;
    }

    irq_routines[irq_num] = handler;
}

/* http://www.thesatya.com/8259.html */
void pic_init(void)
{
    /* init icw1 */
    outb(PIC_MASTER_CMD_PORT,  0x11);
    outb(PIC_SLAVE_CMD_PORT,   0x11);

    /* init icw2, master & slave vector offsets in IDT */
    outb(PIC_MASTER_DATA_PORT, 0x20); /* index 32 in IDT */
    outb(PIC_SLAVE_DATA_PORT,  0x28); /* index 40 in IDT */

    /* init icw3 */
    outb(PIC_MASTER_DATA_PORT, 0x04);
    outb(PIC_SLAVE_DATA_PORT,  0x02);

    /* icw4 init, mode 8086 */
    outb(PIC_MASTER_DATA_PORT, 0x01);
    outb(PIC_SLAVE_DATA_PORT,  0x01);

    /* mask interrupts */
    outb(PIC_MASTER_DATA_PORT, 0x00);
    outb(PIC_SLAVE_DATA_PORT,  0x00);

    idt_set_gate((unsigned long int)irq0,  0x08, 0x8e, &IDT[32]);
    idt_set_gate((unsigned long int)irq1,  0x08, 0x8e, &IDT[33]);
    idt_set_gate((unsigned long int)irq2,  0x08, 0x8e, &IDT[34]);
    idt_set_gate((unsigned long int)irq3,  0x08, 0x8e, &IDT[35]);
    idt_set_gate((unsigned long int)irq4,  0x08, 0x8e, &IDT[36]);
    idt_set_gate((unsigned long int)irq5,  0x08, 0x8e, &IDT[37]);
    idt_set_gate((unsigned long int)irq6,  0x08, 0x8e, &IDT[38]);
    idt_set_gate((unsigned long int)irq7,  0x08, 0x8e, &IDT[39]);
    idt_set_gate((unsigned long int)irq8,  0x08, 0x8e, &IDT[40]);
    idt_set_gate((unsigned long int)irq9,  0x08, 0x8e, &IDT[41]);
    idt_set_gate((unsigned long int)irq10, 0x08, 0x8e, &IDT[42]);
    idt_set_gate((unsigned long int)irq11, 0x08, 0x8e, &IDT[43]);
    idt_set_gate((unsigned long int)irq12, 0x08, 0x8e, &IDT[44]);
    idt_set_gate((unsigned long int)irq13, 0x08, 0x8e, &IDT[45]);
    idt_set_gate((unsigned long int)irq14, 0x08, 0x8e, &IDT[46]);
    idt_set_gate((unsigned long int)irq15, 0x08, 0x8e, &IDT[47]);
}

void pic_ack_interrupt(int irq_num)
{
    if (irq_num >= 40)
        outb(PIC_SLAVE_CMD_PORT, PIC_ACK);
    outb(PIC_MASTER_CMD_PORT, PIC_ACK);
}

/* irq raised by master -> acknowledge master
 * irq raised by slave, -> acknowledge both slave AND master */
void pic_handler(struct cpu_state *cpu_state)
{
    void (*handler)(struct cpu_state *cpu_state);

    int irq_index = cpu_state->isr_num - 32;
    if (irq_index >= 16) {
        kdebug("%x", irq_index);
        kpanic("no handler for this interrupt!");
    }

    if ((handler = irq_routines[irq_index]))
        handler(cpu_state);

    pic_ack_interrupt(cpu_state->isr_num);
}
