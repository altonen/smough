#include <arch/amd64/cpu.h>
#include <kernel/irq.h>
#include <kernel/kpanic.h>
#include <kernel/kassert.h>
#include <kernel/util.h>

#define MAX_INT      256
#define MAX_HANDLERS  16

typedef struct irq_handler irq_handler_t;

extern uint32_t amd64_page_fault_handler(void *ctx);
extern uint32_t amd64_general_protection_fault_handler(void *ctx);

static struct irq_handler {
    int installed;
    struct {
        uint32_t (*handler)(void *);
        void *ctx;
    } handlers[MAX_HANDLERS];
} handlers[MAX_INT];

const char *interrupts[] = {
    "division by zero",            "debug exception",          "non-maskable interrupt",
    "breakpoint",                  "into detected overflow",   "out of bounds",
    "invalid opcode",              "no coprocessor",           "double fault",
    "coprocessor segment overrun", "bad tss",                  "segment not present",
    "stack fault",                 "general protection fault", "page fault",
    "unknown interrupt",           "coprocessor fault",        "alignment check",
    "machine check",               "simd floating point",      "virtualization",
};

void irq_init(void)
{
    kmemset(handlers, 0, sizeof(handlers));

    handlers[VECNUM_PAGE_FAULT].installed           = 1;
    handlers[VECNUM_PAGE_FAULT].handlers[0].handler = amd64_page_fault_handler;
    handlers[VECNUM_PAGE_FAULT].handlers[0].ctx     = NULL;

    handlers[VECNUM_GPF].installed           = 1;
    handlers[VECNUM_GPF].handlers[0].handler = amd64_general_protection_fault_handler;
    handlers[VECNUM_GPF].handlers[0].ctx     = NULL;
}

void interrupt_handler(cpu_state_t *cpu_state)
{
    if (cpu_state->isr_num > MAX_INT)
        kpanic("isr number is too high");

    if (handlers[cpu_state->isr_num].installed) {
        uint32_t ret;

        for (int i = 0; i < handlers[cpu_state->isr_num].installed; ++i) {
            ret = handlers[cpu_state->isr_num].handlers[i].handler(
                    handlers[cpu_state->isr_num].handlers[i].ctx ?
                        handlers[cpu_state->isr_num].handlers[i].ctx :
                        cpu_state
            );

            if (ret == IRQ_HANDLED)
                break;
        }

        return;
    }

    switch (cpu_state->isr_num) {
        case 0x00: case 0x01: case 0x02: case 0x03:
        case 0x04: case 0x05: case 0x06: case 0x07:
        case 0x08: case 0x09: case 0x0a: case 0x0b:
        case 0x0c: case 0x0f: case 0x10: case 0x11:
        case 0x12: case 0x14:
            kpanic(interrupts[cpu_state->isr_num]);
            break;

        default:
            kpanic("unsupported interrut!");
            break;
    }
}
void irq_install_handler(int num, uint32_t (*handler)(void *), void *ctx)
{
    kassert((num >= 0 && num < MAX_INT) && (handler != NULL));
    kassert(handlers[num].installed < MAX_HANDLERS);

    kprint("irq - installing irq handler, num %d, handler 0x%x, ctx 0x%x\n", num, handler, ctx);

    handlers[num].handlers[handlers[num].installed].handler = handler;
    handlers[num].handlers[handlers[num].installed].ctx     = ctx;
    handlers[num].installed++;
}

void irq_uninstall_handler(int num, uint32_t (*handler)(void *))
{
    kassert((num >= 0 && num < MAX_INT));

    kprint("irq - uninstalling irq handler, num %d, handler 0x%x\n", num, handler);

    for (int i = 0; i < handlers[num].installed; ++i) {
        if (handlers[num].handlers[i].handler == handler)
            handlers[num].handlers[i].handler = NULL;
    }
}
