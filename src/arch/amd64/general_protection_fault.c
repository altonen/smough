#include <arch/amd64/cpu.h>
#include <kernel/percpu.h>
#include <kernel/kprint.h>

uint32_t amd64_general_protection_fault_handler(void *ctx)
{
    cpu_state_t *cpu_state = (cpu_state_t *)ctx;
    uint32_t error_number  = cpu_state->err_num;

    kprint("\ngeneral protection fault\n");
    kprint("error number: %d 0x%x\n", error_number, error_number);
    kprint("cpuid: %u\n", get_thiscpu_id());

    switch ((error_number >> 1) & 0x3) {
        case 0b00:
            kprint("faulty descriptor in gdt!\n");
            break;

        case 0b01:
            kprint("faulty descriptor in idt!\n");
            break;

        case 0b10:
            kprint("faulty descriptor in ldt!\n");
            break;

        case 0b11:
            kprint("faulty descriptor in idt!\n");
            break;
    }

    uint32_t desc_index = (error_number >> 3) & 0x1fff;
    kprint("faulty descriptor index: %u\n\n", desc_index);

    for (;;);
}
