#ifndef __AMD64_MMU_TYPES_H__
#define __AMD64_MMU_TYPES_H__

#include <stdint.h>
#include <kernel/kassert.h>

#define PAGE_SIZE 4096
#define KPSTART   0x0000000000100000
#define KVSTART   0xffffffff80100000
#define KPML4I    511

enum MM_PAGE_FLAGS {
    MM_NO_FLAGS   = 0,
    MM_PRESENT    = 1,
    MM_READWRITE  = 1 << 1,
    MM_READONLY   = 0 << 1,
    MM_USER       = 1 << 2,
    MM_WR_THROUGH = 1 << 3,
    MM_D_CACHE    = 1 << 4,
    MM_ACCESSED   = 1 << 5,
    MM_SIZE_4MB   = 1 << 6,
    MM_2MB        = 1 << 7,
    MM_COW        = 1 << 9
};

static inline uint64_t amd64_get_cr3(void)
{
    uint64_t address;

    asm volatile ("mov %%cr3, %%rax \n"
                  "mov %%rax, %0" : "=r" (address));

    return address;
}

static inline void amd64_set_cr3(uint64_t address)
{
    asm volatile ("mov %0, %%rax \n"
                  "mov %%rax, %%cr3" :: "r" (address));
}

static inline void amd64_flush_tlb(void)
{
    asm volatile ("mov %cr3, %rax \n"
                  "mov %rax, %cr3");
}

/* convert a physical address to a virtual address */
static inline uint64_t *amd64_p_to_v(uint64_t paddr)
{
    return (uint64_t *)(paddr + (KVSTART - KPSTART));
}

static inline uint64_t amd64_v_to_p(void *vaddr)
{
    kassert((uint64_t)vaddr > (KVSTART - KPSTART));

    return ((uint64_t)vaddr - (KVSTART - KPSTART));
}

// initialize the archictecture-specific page directories
int mm_native_init(void);

// map a physical address `paddr` point to virtual address 'vaddr'
void amd64_map_page(uint64_t paddr, uint64_t vaddr, int flags);

// build page directory
void *amd64_build_dir(void);

// map a physical address `paddr` point to virtual address 'vaddr'
// where `dir` points to a virtualized PML4 address
void amd64_map_page_to_dir(uint64_t *pml4, uint64_t paddr, uint64_t vaddr, int flags);

// duplicate page directory
//
// create a duplicate of the page directory that is located in cr3, making an identical
// copy of the one and returning pointer to the pml4 of the copy.
//
// mark all leaf pages as copy-on-write and read-only, allowing efficient use of memory.
void *amd64_duplicate_dir(void);

#endif /* __AMD64_MMU_TYPES_H__ */
