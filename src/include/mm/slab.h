#ifndef __SLAB_H__
#define __SLAB_H__

#include <mm/types.h>

typedef struct mm_cache mm_cache_t;

/* initialize the slab allocator using boot memory allocator */
int mm_slab_preinit(void);

/* allocate a slab cache
 *
 * `size` - cache element item size (0 < `size` <= 4096) */
mm_cache_t *mm_cache_create(size_t size);

/* deallocate a slab cache
 *
 * `cache` - non-null pointer to a slab cache */
int mm_cache_destroy(mm_cache_t *cache);

/* allocate entry from a slab cache
 *
 * `cache` - non-null pointer to a slab cache */
void *mm_cache_alloc_entry(mm_cache_t *cache);

/* deallocate entry from a slab cache
 *
 * `cache` - non-null pointer to a slab cache
 * `entry` - non-null pointer to the entry */
int mm_cache_free_entry(mm_cache_t *cache, void *entry);

#endif /* __SLAB_H__ */
