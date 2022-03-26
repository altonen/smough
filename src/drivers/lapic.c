#include <arch/amd64/cpu.h>
#include <drivers/lapic.h>
#include <drivers/pit.h>
#include <kernel/acpi/acpi.h>
#include <kernel/common.h>
#include <kernel/idt.h>
#include <kernel/io.h>
#include <kernel/irq.h>
#include <kernel/kassert.h>
#include <mm/types.h>
#include <errno.h>

/* Local APIC general defines */
#define IA32_APIC_BASE          0x0000001b  /* MSR index */
#define IA32_LAPIC_MSR_BASE     0xfffff000  /* Base addr mask */
#define IA32_LAPIC_MSR_ENABLE   0x00000800  /* Global enable */

/* Local APIC register related defines */
#define LAPIC_REG_TPR      0x0080  /* task priority  */
#define LAPIC_REG_EOI      0x00b0  /* end of interrupt  */
#define LAPIC_REG_DFR      0x00e0  /* destination format  */
#define LAPIC_REG_SVR      0x00f0  /* spurious interrupt  */
#define LAPIC_REG_TMR      0x0180  /* trigger mode */
#define LAPIC_REG_ICR_LO   0x0300  /* int command upper half */
#define LAPIC_REG_ICR_HI   0x0310  /* int command lower half */
#define LAPIC_REG_TIMER    0x0320  /* LVT (timer) */
#define LAPIC_REG_LINT0    0x0350  /* LVT (LINT0) */
#define LAPIC_REG_LINT1    0x0360  /* LVT (LINT1) */
#define LAPIC_REG_ICR      0x0380  /* timer Initial count  */
#define LAPIC_REG_CCR      0x0390  /* timer current count  */
#define LAPIC_REG_CFG      0x03e0  /* timer divide config  */

#define LAPIC_DM_SMI       0x00200  /* delivery mode: SMI */
#define LAPIC_DM_INIT      0x00500  /* delivery mode: INIT */
#define LAPIC_DM_STARTUP   0x00600  /* delivery mode: startup */

/* polarity etc. */
#define LAPIC_LVL_ASSERT    0x04000  /* level: assert */
#define LAPIC_TM_EDGE       0x00000  /* trigger mode: edge */

/* interrupt disabled */
#define LAPIC_INT_DISABLED_MASK  0x10000

/* spurious interrupt register bits */
#define LAPIC_SVR_ENABLE  0x00000100

/* periodic timer */
#define LAPIC_TMR_PERIODIC 0x00020000

static struct {
    uint32_t cpu_id;
    uint32_t lapic_id;
} lapics[MAX_CPU];

static uint8_t *lapic_base     = NULL;
static uint32_t cpu_count      = 0;    /* Number of CPUs */
static uint32_t cpu_init_count = 0;    /* Number of initialized CPUs */

static void __configure_timer(void)
{
    uint16_t ticks;
    uint32_t end;

    kprint("lapic - configure timer\n");

    write_32(lapic_base + LAPIC_REG_CFG, 0x0b);
    write_32(lapic_base + LAPIC_REG_ICR, 0xffffffff);

    /* Configure PIT */
    outb(PIT_CMD,    0xb6);
    outb(PIT_DATA_2, 0xff);
    outb(PIT_DATA_2, 0xff);
    outb(PIT_CHNL_2, inb(PIT_CHNL_2) | 0x1);

    do {
        outb(PIT_CMD, 0xe8);
    } while ((inb(PIT_DATA_2) & 0x80) == 0x80);

    /* sleep 50ms */
    do {
        outb(PIT_CMD, 0x80);
        ticks  =  inb(PIT_DATA_2);
        ticks |= (inb(PIT_DATA_2) << 8);
    } while (ticks > (2 * 65535 - 119318 + 10));

    end = read_32(lapic_base + LAPIC_REG_CCR);
    outb(PIT_CHNL_2, inb(PIT_CHNL_2) & ~0x01);

    write_32(lapic_base + LAPIC_REG_TIMER, LAPIC_TMR_PERIODIC | VECNUM_TIMER);
    write_32(lapic_base + LAPIC_REG_CFG, 0x0b);
    write_32(lapic_base + LAPIC_REG_ICR, ((0xffffffff - end) * 20) / 1000);
}

static uint32_t __svr_handler(void *ctx)
{
    (void)ctx;
    lapic_ack_interrupt();

    return IRQ_HANDLED;
}

static uint32_t __tmr_handler(void *ctx)
{
    (void)ctx;
    lapic_ack_interrupt();

    return IRQ_HANDLED;
}

void lapic_initialize(void)
{
    uint64_t lapic_addr = acpi_get_local_apic_addr();
    uint64_t msr        = get_msr(IA32_APIC_BASE);

    kprint("lapic - initializing local apic\n");

    /* Enable APIC if it's not enabled already */
    if ((msr & IA32_LAPIC_MSR_BASE) != lapic_addr || !(msr & IA32_LAPIC_MSR_ENABLE)) {
        msr = lapic_addr | IA32_LAPIC_MSR_ENABLE;
        set_msr(IA32_APIC_BASE, msr);
    }

    /* Because the Local APIC is above 2GB, we must explicitly map it to address space  */
    lapic_base = (uint8_t *)lapic_addr;
    amd64_map_page(lapic_addr, (uint64_t)lapic_base, MM_PRESENT | MM_READWRITE);

    kprint("lapic - base address 0x%x\n", (uint64_t)lapic_base);

    write_32(lapic_base + LAPIC_REG_DFR, 0xffffffff);
    write_32(lapic_base + LAPIC_REG_TPR, 0);

    kprint("lapic - initialize interrupts\n");

    /* configure Local APIC Timer */
    __configure_timer();

    /* disable logical interrupt lines */
    write_32(lapic_base + LAPIC_REG_LINT0, LAPIC_INT_DISABLED_MASK);
    write_32(lapic_base + LAPIC_REG_LINT1, LAPIC_INT_DISABLED_MASK);

    /* enable Local APIC */
    write_32(lapic_base + LAPIC_REG_SVR, LAPIC_DM_SMI | LAPIC_SVR_ENABLE | VECNUM_SPURIOUS);

    /* acknowledge pending interrupts */
    write_32(lapic_base + LAPIC_REG_EOI, 0);

    irq_install_handler(VECNUM_TIMER,    __tmr_handler, NULL);
    irq_install_handler(VECNUM_SPURIOUS, __svr_handler, NULL);

    kprint("lapic - lapic initialized, lapic count: %d\n", ++cpu_init_count);
}

void lapic_register_dev(int cpu_id, int lapic_id)
{
    kassert(cpu_id >= 0 && cpu_id < MAX_CPU);
    kprint("lapic - register device, cpu id (%d), lapic id (%d)\n", cpu_id, lapic_id);

    lapics[cpu_id].cpu_id    = cpu_id;
    lapics[cpu_id].lapic_id = lapic_id;

    cpu_count++;
}

uint32_t lapic_get_cpu_count(void)
{
    return cpu_count;
}

uint32_t lapic_get_init_cpu_count(void)
{
    return cpu_init_count;
}

int lapic_get_lapic_id(uint32_t cpu)
{
    if (cpu >= cpu_count)
        return -ENXIO;

    return lapics[cpu].lapic_id;
}

void lapic_send_ipi(uint32_t high, uint32_t low)
{
    kprint("lapic - send ipi, high 0x%x, low 0x%x\n", high, low);

    write_32(lapic_base + LAPIC_REG_ICR_HI, high);
    write_32(lapic_base + LAPIC_REG_ICR_LO, low);
}

void lapic_send_init(uint32_t cpu)
{  
    kprint("lapic - send init, cpu %u\n", cpu);

    uint32_t high = (lapics[cpu].lapic_id << 24) & 0xff000000;
    uint32_t low  = LAPIC_DM_INIT | LAPIC_TM_EDGE | LAPIC_LVL_ASSERT;

    lapic_send_ipi(high, low);
}

void lapic_send_sipi(uint32_t cpu, uint32_t vec)
{
    kprint("lapic - send sipi, cpu %u, vec %u\n", cpu, vec);

    uint32_t high = (lapics[cpu].lapic_id << 24) & 0xff000000;
    uint32_t low  = (vec & 0xff) | LAPIC_DM_STARTUP | LAPIC_TM_EDGE | LAPIC_LVL_ASSERT;

    lapic_send_ipi(high, low);
}

void lapic_ack_interrupt(void)
{
    write_32(lapic_base + LAPIC_REG_EOI, 0);
}

uint64_t lapic_get_base(void)
{
    if (!lapic_base)
        return INVALID_ADDRESS;
    return (uint64_t)lapic_base;
}
