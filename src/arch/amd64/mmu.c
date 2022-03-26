#include <kernel/common.h>
#include <kernel/kassert.h>
#include <kernel/util.h>
#include <mm/page.h>
#include <mm/types.h>
#include <stdint.h>

static uint64_t __pml4[512]   __attribute__((aligned(PAGE_SIZE)));
static uint64_t __pdpt[512]   __attribute__((aligned(PAGE_SIZE)));
static uint64_t __pd[512 * 2] __attribute__((aligned(PAGE_SIZE)));
static uint64_t __pml4_;

#define PML4_ATOEI(addr) (((addr) >> 39) & 0x1FF)
#define PDPT_ATOEI(addr) (((addr) >> 30) & 0x1FF)
#define PD_ATOEI(addr)   (((addr) >> 21) & 0x1FF)
#define PT_ATOEI(addr)   (((addr) >> 12) & 0x1FF)
#define V_TO_P(addr)     ((uint64_t)addr - KVSTART + KPSTART)

int mm_native_init(void)
{
    kmemset(__pml4, 0, sizeof(__pml4));
    kmemset(__pdpt, 0, sizeof(__pdpt));
    kmemset(__pd,   0, sizeof(__pd));

    /* map also the lower part of the address space so our boot stack still works
     * It only has to work until we switch to init task */
    __pml4[PML4_ATOEI(KVSTART)] = __pml4[0] = V_TO_P(&__pdpt) | MM_PRESENT | MM_READWRITE;

    /* map the first 2GB of address space */
    for (size_t i = 0; i < 512 * 2; ++i)
        __pd[i] = (i * (1 << 21)) | MM_PRESENT | MM_READWRITE | MM_2MB;

    for (size_t i = 0; i < 2; ++i)
        __pdpt[PDPT_ATOEI(KVSTART) + i] = __pdpt[i] = V_TO_P(&__pd[i * 512]) | MM_PRESENT | MM_READWRITE;

    amd64_set_cr3(V_TO_P(__pml4));
    return 0;
}

static uint64_t __alloc_page_directory_entry(void)
{
    uint64_t addr = mm_block_alloc(MM_ZONE_NORMAL, 1, MM_NO_FLAGS);
    kmemset((void *)amd64_p_to_v(addr), 0, PAGE_SIZE);

    return addr | MM_PRESENT | MM_READWRITE;
}

void amd64_map_page(uint64_t paddr, uint64_t vaddr, int flags)
{
    kassert(PAGE_ALIGNED(paddr));
    kassert(PAGE_ALIGNED(vaddr));

    uint64_t *pml4 = amd64_p_to_v(amd64_get_cr3());
    uint64_t pml4i = (vaddr >> 39) & 0x1ff;
    uint64_t pdpti = (vaddr >> 30) & 0x1ff;
    uint64_t pdi   = (vaddr >> 21) & 0x1ff;
    uint64_t pti   = (vaddr >> 12) & 0x1ff;

    if (!(pml4[pml4i] & MM_PRESENT))
        pml4[pml4i] = __alloc_page_directory_entry();
    pml4[pml4i] |= flags;

    uint64_t *pdpt = amd64_p_to_v(pml4[pml4i] & ~0xfff);

    if (!(pdpt[pdpti] & MM_PRESENT))
        pdpt[pdpti] = __alloc_page_directory_entry();
    pdpt[pdpti] |= flags;

    uint64_t *pd = amd64_p_to_v(pdpt[pdpti] & ~0xfff);

    if (!(pd[pdi] & MM_PRESENT))
        pd[pdi] = __alloc_page_directory_entry();
    pd[pdi] |= flags;

    uint64_t *pt = amd64_p_to_v(pd[pdi] & ~0xfff);
    pt[pti]      = paddr | flags | MM_PRESENT;
}
