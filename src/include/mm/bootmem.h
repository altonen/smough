#ifndef __BOOTMEM_H__
#define __BOOTMEM_H__

#include <stdint.h>
#include <stddef.h>

/* initialize the boot memory manager
 *
 * this is a temporary memory manager that is used to
 * initialize the actual memory manager but because
 * the MMU requires access to heap, there has to be acess
 * to a pre-init heap which is used to initialize the MMU
 * and actual heap.
 *
 * `arg` - pointer to multiboot2 information */
int mm_bootmem_init(void *arg);

/* allocate `npages` of memory from the boot memory allocator
 *
 * the returned memory is physical memory and must be converted
 * to a virtual memory before it's used */
uint64_t mm_bootmem_alloc_block(size_t npages);

#endif /* __BOOTMEM_H__ */
