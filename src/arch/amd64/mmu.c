#include <drivers/bus/pci.h>
#include <drivers/gfx/vbe.h>
#include <drivers/ioapic.h>
#include <drivers/lapic.h>
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

void amd64_map_page_to_dir(uint64_t *pml4, uint64_t paddr, uint64_t vaddr, int flags)
{
    kassert(PAGE_ALIGNED(paddr) && paddr != INVALID_ADDRESS);
    kassert(PAGE_ALIGNED(vaddr) && vaddr != INVALID_ADDRESS);

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

void amd64_map_page(uint64_t paddr, uint64_t vaddr, int flags)
{
    amd64_map_page_to_dir(amd64_p_to_v(amd64_get_cr3()), paddr, vaddr, flags);
}

void *amd64_build_dir(void)
{
    uint64_t pml4_p  = mm_page_alloc(MM_ZONE_NORMAL, 0);
    uint64_t *pml4_v = amd64_p_to_v(pml4_p);

	kassert(pml4_p != INVALID_ADDRESS);

    for (size_t i = 0; i < 511; ++i)
        pml4_v[i] = 0;

	// map kernel to address space
    pml4_v[PML4_ATOEI(KVSTART)] = __pml4[PML4_ATOEI(KVSTART)];

	// TODO: device subsystem
    unsigned long lapic  = lapic_get_base();
    unsigned long ioapic = ioapic_get_base();

    kassert(lapic  != INVALID_ADDRESS);
    kassert(ioapic != INVALID_ADDRESS);

    amd64_map_page_to_dir(pml4_v, lapic,  lapic,  MM_PRESENT | MM_READWRITE);
    amd64_map_page_to_dir(pml4_v, ioapic, ioapic, MM_PRESENT | MM_READWRITE);

    pci_dev_t *dev = pci_get_dev(VBE_VENDOR_ID, VBE_DEVICE_ID);

    if (dev) {
        void *vga_mem = (uint8_t *)((uint64_t)dev->bar0 - 8);

		for (size_t i = 0; i < 4096; ++i) {
            amd64_map_page_to_dir(
                pml4_v,
                (uint64_t)vga_mem,
                (uint64_t)vga_mem,
                MM_PRESENT | MM_READWRITE
            );
		}
    }

    return pml4_v;
}

void *amd64_duplicate_dir(void)
{
    uint64_t *pml4_cv = amd64_build_dir();             // copy, virtual
    uint64_t *pml4_ov = amd64_p_to_v(amd64_get_cr3()); // original, virtual
    uint64_t *pdpt_ov = NULL, *pdpt_cv = NULL;
    uint64_t *pd_ov   = NULL, *pd_cv   = NULL;
    uint64_t *pt_ov   = NULL, *pt_cv   = NULL;

    // map all user pages, kernel stays untouched
    for (size_t pml4i = 0; pml4i < 511; ++pml4i) {
        if (pml4_ov[pml4i] & MM_PRESENT) {

            // create new pml4 entry and set up pdpt pointers
            pml4_cv[pml4i] = __alloc_page_directory_entry() | MM_PRESENT | MM_USER;
            pdpt_ov        = amd64_p_to_v(pml4_ov[pml4i] & ~(PAGE_SIZE - 1));
            pdpt_cv        = amd64_p_to_v(pml4_cv[pml4i] & ~(PAGE_SIZE - 1));

            // level 2
            for (size_t pdpti = 0; pdpti < 512; ++pdpti)  {
                if (pdpt_ov[pdpti] & MM_PRESENT) {

                    // create new pdpt entry and set up pd pointers
                    pdpt_cv[pdpti] = __alloc_page_directory_entry() | MM_PRESENT | MM_USER;
                    pd_ov          = amd64_p_to_v(pdpt_ov[pdpti] & ~(PAGE_SIZE - 1));
                    pd_cv          = amd64_p_to_v(pdpt_cv[pdpti] & ~(PAGE_SIZE - 1));

                    // mark all pages as copy-on-write (COW)
                    //
                    // this allows user-mode processes to efficiently share the same address
                    // space and only make copy of the pages that they touch. As can be seen
                    // below, all pages are marked as `MM_READONLY` which means that when kernel
                    // tries to write to it, a page fault is raised and the page fault handler
                    // will create a copy of the page for the caller but this time without
                    // `MM_COW | MM_READONLY` flags
                    for (size_t pdi = 0; pdi < 512; ++pdi)  {
                        if (pd_ov[pdi] & MM_PRESENT) {

                            // 2MB pages should not be mapped like 4KB pages
                            if ((pd_ov[pdi] & MM_2MB)) {
                                pd_ov[pdi] &= ~(PAGE_SIZE - 1); // reset flags
                                pd_ov[pdi] |= (MM_COW | MM_READONLY | MM_PRESENT | MM_USER | MM_2MB);
                                pd_cv[pdi]  = pd_ov[pdi];
                            }

                            // create new pd entry and set up pt pointers
                            pd_cv[pdi] = __alloc_page_directory_entry() | MM_PRESENT | MM_USER;
                            pt_ov      = amd64_p_to_v(pd_ov[pdi] & ~(PAGE_SIZE - 1));
                            pt_cv      = amd64_p_to_v(pd_cv[pdi] & ~(PAGE_SIZE - 1));

                            for (size_t pti = 0; pti < 512; ++pti) {
                                if (pt_ov[pti] & MM_PRESENT) {
                                    pt_ov[pti] &= ~(PAGE_SIZE - 1); // reset flags
                                    pt_ov[pti] |= (MM_COW | MM_READONLY | MM_PRESENT | MM_USER);
                                    pt_cv[pti]  = pt_ov[pti];
                                }
                            }
                        }
                    }
                }
            }
        }
    }

	// TODO: wasteful
    amd64_flush_tlb();

    return pml4_cv;
}
