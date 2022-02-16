#include <arch/amd64/cpu.h>
#include <kernel/irq.h>
#include <kernel/kpanic.h>

#define MAX_INT 256

const char *interrupts[] = {
    "division by zero",            "debug exception",          "non-maskable interrupt",
    "breakpoint",                  "into detected overflow",   "out of bounds",
    "invalid opcode",              "no coprocessor",           "double fault",
    "coprocessor segment overrun", "bad tss",                  "segment not present",
    "stack fault",                 "general protection fault", "page fault",
    "unknown interrupt",           "coprocessor fault",        "alignment check",
    "machine check",               "simd floating point",      "virtualization",
};

void interrupt_handler(isr_regs_t *cpu_state)
{
    if (cpu_state->isr_num > MAX_INT)
        kpanic("ISR number is too high");

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
