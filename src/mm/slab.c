#include <kernel/common.h>
#include <kernel/kassert.h>
#include <kernel/kprint.h>
#include <kernel/util.h>
#include <lib/list.h>
#include <mm/bootmem.h>
#include <mm/heap.h>
#include <mm/slab.h>
#include <errno.h>

struct cache_free_chunk {
    void *mem;
    list_head_t list;
};

typedef struct cache_fixed_entry {
    size_t num_free;
    void *next_free;
    void *mem;

    list_head_t list;
} cfe_t;

struct mm_cache {
    size_t item_size;
    size_t capacity;

    struct cache_fixed_entry *free_list;
    struct cache_fixed_entry *used_list;
    struct cache_free_chunk *free_chunks;
};

static list_head_t free_list;

static cfe_t *__alloc_cfe(size_t item_size)
{
    kassert(item_size != 0);

    cfe_t *e = container_of(free_list.next, struct cache_fixed_entry, list);

    list_remove(&e->list);
    e->num_free = (PAGE_SIZE / item_size) - 1;

    return e;
}

int mm_slab_preinit(void)
{
    kprint("slab: initializing slab with bootmem\n");

    list_init_null(&free_list);

    /* allocate 16 KB for booting */
    for (int i = 0; i < 2; ++i) {
        uint64_t mem = mm_bootmem_alloc_block(1);
        void *mem_v  = amd64_p_to_v(mem);

        cfe_t *entry = kzalloc(sizeof(cfe_t));
        kassert(entry != NULL);

        entry->mem       = mem_v;
        entry->next_free = mem_v;
        entry->num_free  = PAGE_SIZE;

        list_init_null(&entry->list);
        list_append(&free_list, &entry->list);
    }

    return 0;
}

mm_cache_t *mm_cache_create(size_t size)
{
    kassert(size > 0 && size <= PAGE_SIZE);

    mm_cache_t *c = NULL;

    if (!(c = kzalloc(sizeof(mm_cache_t))))
        return NULL;

    c->item_size = MULTIPLE_OF_2(size);
    c->capacity  = PAGE_SIZE / c->item_size;

    c->free_list = __alloc_cfe(c->item_size);
    c->used_list = NULL;

    return c;
}

int mm_cache_destroy(mm_cache_t *cache)
{
    kassert(cache != NULL);

    if (cache->used_list) {
        kprint("slab: cache still in use, unable to destroy it!\n");
        return -EBUSY;
    }

    kfree(cache);
    return 0;
}

void *mm_cache_alloc_entry(mm_cache_t *cache)
{
    kassert(cache != NULL);

    /* if there are any free chunks left, try to use them first and return early */
    if (cache->free_chunks) {
        void *ret = cache->free_chunks->mem;

        /* is there only one free chunk? */
        if (!cache->free_chunks->list.next) {
            cache->free_chunks = NULL;
        } else {
            list_head_t *next_chunk = cache->free_chunks->list.next;
            list_remove(&cache->free_chunks->list);
            cache->free_chunks = container_of(next_chunk, struct cache_free_chunk, list);
        }

        return ret;
    }

    if (!cache->free_list) {
        cache->free_list = __alloc_cfe(cache->item_size);
        cache->capacity += PAGE_SIZE / cache->item_size;
    }
    void *ret = cache->free_list->next_free;

    /* if last element, set free_list to NULL and append cache_entry to used list
     * else set next_free to point to next free element in cache->free_list->mem */
    if (!--cache->free_list->num_free) {
        if (!cache->used_list)
            cache->used_list = cache->free_list;
        else
            list_append(&cache->used_list->list, &cache->free_list->list);
        cache->free_list = NULL;
    } else {
        cache->free_list->next_free = (uint8_t *)cache->free_list->next_free + cache->item_size;
    }

    kmemset(ret, 0, cache->item_size);
    return ret;
}

int mm_cache_free_entry(mm_cache_t *cache, void *entry)
{
    kassert(cache != NULL);
    kassert(entry != NULL);

    struct cache_free_chunk *cfc = kmalloc(sizeof(struct cache_free_chunk));

    list_init_null(&cfc->list);
    cfc->mem = entry;

    if (!cache->free_chunks)
        cache->free_chunks = cfc;
    else
        list_append(&cache->free_chunks->list, &cfc->list);

    return 0;
}
