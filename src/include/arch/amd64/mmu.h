#ifndef __AMD64_MMU_TYPES_H__
#define __AMD64_MMU_TYPES_H__

#include <stdint.h>

#define PAGE_SIZE 4096
#define KPSTART   0x0000000000100000
#define KVSTART   0xffffffff80100000
#define KPML4I    511

enum MM_PAGE_FLAGS {
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

/* Initialize the archictecture-specific page directories */
int mm_native_init(void);

#endif /* __AMD64_MMU_TYPES_H__ */
