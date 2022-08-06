#include <arch/amd64/cpu.h>
#include <kernel/kprint.h>

#define REG(reg) reg, reg

void amd64_dump_registers(cpu_state_t *cpu_state)
{
    if (!cpu_state) {
        uint64_t rax, rbx, rcx, rdx, rdi, cr3, cr2;

        asm volatile ("mov %%rax, %0\n\
                       mov %%rbx, %1\n\
                       mov %%rcx, %2\n\
                       mov %%rdx, %3\n\
                       mov %%rdi, %4\n\
                       mov %%cr2, %%rax\n\
                       mov %%rax, %5\n\
                       mov %%cr3, %%rax\n\
                       mov %%rax, %6"
                       : "=g"(rax), "=g"(rbx), "=g"(rcx), "=g"(rdx),
                         "=g"(rdi), "=g"(cr2), "=g"(cr3));

        kprint("\nregister contents:\n");
        kprint("\trax: 0x%016x %20u\n\trbx: 0x%016x %20u\n\trcx: 0x%016x %20u\n"
               "\trdx: 0x%016x %20u\n\trdi: 0x%016x %20u\n\tcr2: 0x%016x %20u\n"
               "\tcr3: 0x%016x %20u\n\n", REG(rax), REG(rbx), REG(rcx),
               REG(rdx), REG(rdi), REG(cr2), REG(cr3));
    } else {
        kprint("\nRegister contents:\n");
        kprint("\trax: 0x%016x %20u\n"
               "\trcx: 0x%016x %20u\n"
               "\trdx: 0x%016x %20u\n"
               "\trbx: 0x%016x %20u\n"
               "\trsi: 0x%016x %20u\n"
               "\trdi: 0x%016x %20u\n"
               "\trbp: 0x%016x %20u\n"
               "\trsp: 0x%016x %20u\n"
               "\trip: 0x%016x %20u\n",     cpu_state->rax, cpu_state->rax,
            cpu_state->rcx, cpu_state->rcx, cpu_state->rdx, cpu_state->rdx,
            cpu_state->rbx, cpu_state->rbx, cpu_state->rsi, cpu_state->rsi,
            cpu_state->rdi, cpu_state->rdi, cpu_state->rbp, cpu_state->rbp,
            cpu_state->rsp, cpu_state->rsp, cpu_state->rip, cpu_state->rip);
    }
}

