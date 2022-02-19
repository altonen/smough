#ifndef __MMU_H__
#define __MMU_H__

#include <sys/types.h>

/* Initialize the memory management unit
 *
 * # Arguments
 * `arg` - pointer to multiboot2 information */
int mm_init(void *arg);

#endif /* __MMU_H__ */
