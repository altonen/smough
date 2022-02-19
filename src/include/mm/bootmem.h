#ifndef __BOOTMEM_H__
#define __BOOTMEM_H__

/* Initialize the boot memory manager
 *
 * This is a temporary memory manager that is used to
 * initialize the actual memory manager but because
 * the MMU requires access to heap, there has to be acess
 * to a pre-init heap which is used to initialize the MMU
 * and actual heap.
 *
 * `arg` - pointer to multiboot2 information */
int mm_bootmem_init(void *arg);

#endif /* __BOOTMEMH__ */
