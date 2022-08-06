#include <arch/amd64/mmu.h>
#include <kernel/common.h>
#include <kernel/irq.h>
#include <kernel/kpanic.h>
#include <kernel/kprint.h>
#include <kernel/percpu.h>
#include <kernel/util.h>
#include <mm/mmu.h>
#include <mm/page.h>

uint32_t amd64_page_fault_handler(void *ctx)
{
    cpu_state_t *cpu_state = (cpu_state_t *)ctx;

    uint64_t cr2   = 0;
    uint64_t cr3   = 0;
    uint32_t error = cpu_state->err_num;

    asm volatile ("mov %%cr3, %0" : "=r"(cr3));
    asm volatile ("mov %%cr2, %0" : "=r"(cr2));

    unsigned long pml4i = (cr2 >> 39) & 0x1ff;
    unsigned long pdpti = (cr2 >> 30) & 0x1ff;
    unsigned long pdi   = (cr2 >> 21) & 0x1ff;
    unsigned long pti   = (cr2 >> 12) & 0x1ff;

    uint64_t *pml4 = amd64_p_to_v(cr3);

    if (!(pml4[pml4i] & MM_PRESENT))
        goto error;

    uint64_t *pdpt = amd64_p_to_v(pml4[pml4i] & ~(PAGE_SIZE - 1));

    if (!(pdpt[pdpti] & MM_PRESENT))
        goto error;

    uint64_t *pd = amd64_p_to_v(pdpt[pdpti] & ~(PAGE_SIZE - 1));

    if (!(pd[pdi] & MM_PRESENT))
        goto error;

    uint64_t *pt = amd64_p_to_v(pd[pdi] & ~(PAGE_SIZE - 1));

    // TODO: handle copy on write

error:;
    const char *s[3] = {
        ((error >> 1) & 0x1) ? "page write"       : "page read",
        ((error >> 0) & 0x1) ? "protection fault" : "not present",
        ((error >> 2) & 0x1) ? "user"             : "supervisor"
    };

    kprint("\n\n----------------------------------\n");
    kprint("%s, %s (%s)\n", s[0], s[1], s[2]);

    if ((error >> 3) & 0x1)
        kprint("cpu read 1 from reserved field\n");

    if ((error >> 4) & 0x1)
        kprint("faulted due to instruction fetch (nxe set?)\n");

    kprint("\nfaulting address: 0x%08x %10u\n", cr2, cr2);
    kprint("pml4 index:       0x%08x %10u\n"
           "pdpt index:       0x%08x %10u\n"
           "pd index:         0x%08x %10u\n"
           "pt index:         0x%08x %10u\n"
           "offset:           0x%08x %10u\n\n",
           pml4i, pml4i, pdpti, pdpti, pdi, pdi, pti, pti, cr2 & 0xfff, cr2 & 0xfff);

    kprint("cpuid:            0x%08x %10u\n", get_thiscpu_id(), get_thiscpu_id());
    kprint("cr3:              0x%08x %10u\n", cr3, cr3);

    amd64_dump_registers(cpu_state);

    for (;;);
}
