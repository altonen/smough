#ifndef __MMU_TYPES_H__
#define __MMU_TYPES_H__

#include <arch/amd64/mmu.h>
#include <lib/list.h>
#include <sys/types.h>

#define INVALID_ADDRESS 0xffffffffffffffff
#define BUDDY_MAX_ORDER 0x10
#define PAGE_SHIFT      0x0c

enum MM_ZONES {
    MM_ZONE_DMA,    /* 0  MB - 16 MB */
    MM_ZONE_NORMAL, /* 16 MB - 2 GB */
    MM_ZONE_HIGH,   /* 2  GB - */
};

enum MM_ZONE_RANGES {
    MM_ZONE_DMA_START    = 0x0000000000000000,
    MM_ZONE_DMA_END      = 0x0000000000ffffff,
    MM_ZONE_NORMAL_START = 0x0000000001000000,
    MM_ZONE_NORMAL_END   = 0x000000007fffffff,
    MM_ZONE_HIGH_START   = 0x0000000080000000,
    MM_ZONE_HIGH_END     = 0xffffffffffffffff,
};

enum MM_PAGE_TYPES {
    MM_PT_INVALID = 0 << 0,
    MM_PT_FREE    = 1 << 0,
    MM_PT_IN_USE  = 1 << 1,
};

typedef struct page {
    list_head_t list;
    uint8_t type:2;  /* page type */
    uint8_t order:5; /* block order [0, BUDDY_MAX_ORDER[ */
    uint8_t first:1; /* first block of range? */
} page_t;

#endif /* __MMU_TYPES_H__ */
