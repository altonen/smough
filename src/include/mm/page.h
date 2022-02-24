#ifndef __PAGE_H__
#define __PAGE_H__

#include <stdint.h>
#include <stddef.h>

/* initialize memory zones */
void mm_zones_init(void *arg);

/* allocate block of physical memory */
uint64_t mm_block_alloc(uint32_t memzone, uint32_t order, int flags);

/* allocate one page of physical memory */
uint64_t mm_page_alloc(uint32_t memzone, int flags);

/* free a block of physical memory */
int mm_block_free(uint64_t address, uint32_t order);

/* free a page of physical memory */
int mm_page_free(uint64_t address);

/* claim a page of physical memory at `addres` for page frame allocator */
void mm_claim_page(uint64_t address);

/* claim a range of physical memory for page frame allocator */
void mm_claim_range(uint64_t address, size_t len);

#endif /* __PAGE_H__ */
