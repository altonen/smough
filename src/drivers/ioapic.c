#include <kernel/io.h>
#include <drivers/ioapic.h>
#include <kernel/common.h>
#include <kernel/irq.h>
#include <kernel/kassert.h>
#include <kernel/kprint.h>
#include <kernel/pic.h>
#include <mm/types.h>

#define MAX_IOAPIC          20
#define IOAPIC_IOREGSEL   0x00
#define IOAPIC_IOWIN      0x10
#define IOAPIC_REG_ID     0x00
#define IOAPIC_REG_VER    0x01
#define IOAPIC_REG_TABLE  0x10
#define IOAPIC_INT_DISABLED   0x00010000

static struct {
    int num_apics;

    struct {
        uint8_t  id;
        uint8_t *base;
        uint8_t *mapped;
        uint32_t intr_base;
    } apics[MAX_IOAPIC];

} io_apic;

static uint32_t __read_reg(uint8_t *base, uint32_t reg)
{
    write_32(base + IOAPIC_IOREGSEL, reg);
    return read_32(base + IOAPIC_IOWIN);
}

static void __write_reg(uint8_t *base, uint32_t reg, uint32_t value)
{
    write_32(base + IOAPIC_IOREGSEL, reg);
    write_32(base + IOAPIC_IOWIN,    value);
}

void ioapic_initialize_all(void)
{
    kassert(io_apic.num_apics > 0);

    /* disable 8259 PIC by masking all interrupts */
    outb(PIC_MASTER_DATA_PORT, 0xff);
    outb(PIC_SLAVE_DATA_PORT,  0xff);

    /* Initialize all I/O APICs found on the bus and initially disable all IRQs */
    for (int i = 0; i < io_apic.num_apics; ++i) {
        kprint("ioapic - initializing I/O APIC %d\n", io_apic.apics[i].id);

        uint8_t *ioapic_v = (uint8_t *)io_apic.apics[i].base;
        amd64_map_page((uint64_t)ioapic_v, (uint64_t)ioapic_v, MM_PRESENT | MM_READWRITE);

        uint32_t id  = __read_reg(ioapic_v, IOAPIC_REG_ID);
        uint32_t ver = __read_reg(ioapic_v, IOAPIC_REG_VER);

        /* Disabling the interrupt happens in two writes:
         *  - first write  (offset 0) disables the irq line itself
         *  - second write (offset 1) sets the destination cpu of disabled irq to 0 */
        for (size_t intr = 0; intr < ((ver >> 16) & 0xff); ++intr) {
            unsigned offset = IOAPIC_REG_TABLE + 2 * intr + io_apic.apics[i].intr_base;

            __write_reg(ioapic_v, offset + 0, IOAPIC_INT_DISABLED | (VECNUM_IRQ_START + intr));
            __write_reg(ioapic_v, offset + 1, 0);
        }
    }
}

void ioapic_register_dev(uint8_t ioapic_id, uint32_t ioapic_addr, uint32_t intr_base)
{
    kassert(ioapic_id < MAX_IOAPIC);

    kprint("ioapic - registering ioapic device %u 0x%u 0x%x\n", ioapic_id, ioapic_addr, intr_base);

    io_apic.apics[ioapic_id].id        = ioapic_id;
    io_apic.apics[ioapic_id].base      = (uint8_t *)(uint64_t)ioapic_addr;
    io_apic.apics[ioapic_id].intr_base = intr_base;

    io_apic.num_apics++;
}

void ioapic_enable_irq(unsigned cpu, unsigned irq)
{
    kprint("ioapic - enable irq line %u for cpu %u\n", irq, cpu);

    unsigned offset   = IOAPIC_REG_TABLE + 2 * (irq - VECNUM_IRQ_START);
    uint8_t *ioapic_v = (uint8_t *)io_apic.apics[cpu].base;
    amd64_map_page((uint64_t)ioapic_v, (uint64_t)ioapic_v, MM_PRESENT | MM_READWRITE);

    __write_reg(ioapic_v, offset + 0, irq);
    __write_reg(ioapic_v, offset + 1, cpu << 24);
}

uint64_t ioapic_get_base(void)
{
    kassert(io_apic.num_apics == 1);

    return (uint64_t)io_apic.apics[0].base;
}
