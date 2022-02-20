#include <kernel/compiler.h>
#include <kernel/kassert.h>
#include <kernel/kpanic.h>
#include <kernel/kprint.h>
#include <kernel/util.h>
#include <mm/bootmem.h>
#include <mm/types.h>
#include <sys/types.h>
#include <errno.h>

#define SPLIT_THRESHOLD  8
#define HEAP_ARENA_SIZE  2

typedef struct mm_chunk {
    size_t size;
    bool free;
    struct mm_chunk *next;
    struct mm_chunk *prev;
} __packed mm_chunk_t;

typedef struct mm_arena {
    size_t size;
    mm_chunk_t *base;
    struct mm_arena *next;
    struct mm_arena *prev;
} __packed mm_arena_t;

static mm_arena_t __mem;
static int initialized;

static mm_chunk_t *__split_block(mm_chunk_t *block, size_t size)
{
    kassert(block != NULL);
    kassert(block->size >= size);

    if (block->size - size < sizeof(mm_chunk_t) + SPLIT_THRESHOLD)
        return block;

    mm_chunk_t *new = (mm_chunk_t *)((uint8_t *)block + sizeof(mm_chunk_t) + size);

    new->size   = block->size - size - sizeof(mm_chunk_t);
    new->free   = 1;
    block->size = size;

    if (block->next)
        block->next->prev = new;

    new->next   = block->next;
    block->next = new;
    new->prev   = block;

    return block;
}

static mm_chunk_t *__find_free(mm_arena_t *arena, size_t size)
{
    kassert(arena != NULL);
    kassert(size != 0);

    mm_arena_t *iter = arena;

    while (iter) {
        mm_chunk_t *block = iter->base;

        while (block && !(block->free && block->size >= size))
            block = block->next;

        if (block)
            return __split_block(block, size);

        iter = iter->next;
    }

    return NULL;
}

static mm_chunk_t *__merge_blocks_prev(mm_chunk_t *cur, mm_chunk_t *prev)
{
    kassert(cur != NULL);

    if (!prev || !prev->free)
        return cur;

    mm_chunk_t *head = prev;

    head->size = prev->size + cur->size + sizeof(mm_chunk_t);
    head->next = cur->next;

    if (cur->next)
        cur->next->prev = head;

    return __merge_blocks_prev(prev, prev->prev);
}

static void __merge_blocks_next(mm_chunk_t *cur, mm_chunk_t *next)
{
    kassert(cur != NULL);

    if (!next || !next->free)
        return;

    cur->size += next->size + sizeof(mm_chunk_t);
    cur->next  = next->next;

    if (next->next)
        next->next->prev = cur;

    __merge_blocks_next(cur, cur->next);
}

void *kmalloc(size_t size)
{
    mm_chunk_t *block;

    if (!(block = __find_free(&__mem, size)))
        kpanic("out of memory");

    block->free = 0;
    return block + 1;
}

void *kzalloc(size_t size)
{
    void *mem = kmalloc(size);

    kmemset(mem, 0, size);
    return mem;
}

void kfree(void *mem)
{
    mm_chunk_t *block = (mm_chunk_t *)mem - 1;
    block->free = 1;
    block = __merge_blocks_prev(block, block->prev);
    __merge_blocks_next(block, block->next);
}

int mm_heap_preinit(void)
{
    kprint("heap: initializing kernel heap with bootmem\n");

    /* allocate two pages or 16 KB of memory for booting */
    unsigned long mem = mm_bootmem_alloc_block(2);

    if (mem == INVALID_ADDRESS)
        kpanic("failed to allocate memory for heap");

    __mem.base = (mm_chunk_t *)amd64_p_to_v(mem);
    __mem.size = PAGE_SIZE * (1 << 2);

    __mem.base->size = PAGE_SIZE * (1 << 2) - sizeof(mm_chunk_t);
    __mem.base->free = 1;
    __mem.base->next = NULL;
    __mem.base->prev = NULL;

    return 0;
}
