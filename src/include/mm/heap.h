#ifndef __MM_KERNEL_HEAPP_H__
#define __MM_KERNEL_HEAPP_H__

#include <stddef.h>
#include <stdint.h>

/* allocate memory from kernel heap
 *
 * panics if out of memory */
void *kmalloc(size_t size);

/* allocate zeroed-out memory from kernel heap
 *
 * panics if out of memory */
void *kzalloc(size_t size);

/* free an allocated memory object */
void kfree(void *ptr);

/* initialize the kernel heap using boot memory allocator
 *
 * this is a temporary initialization and it's only used
 * so that SLAB and page frame allocator can be initialized */
int mm_heap_preinit(void);

#endif /* __MM_KERNEL_HEAP_H__ */
