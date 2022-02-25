#include <arch/amd64/mmu.h>
#include <kernel/kprint.h>
#include <mm/bootmem.h>
#include <mm/heap.h>
#include <mm/mmu.h>
#include <mm/page.h>
#include <mm/slab.h>

int mm_init(void *arg)
{
    kprint("mmu: initializing memory allocators\n");

    /* initialize the native memory management unit
     * and whatever else the native platform needs */
    (void)mm_native_init();

    (void)mm_bootmem_init(arg);
    (void)mm_heap_preinit();
    (void)mm_slab_preinit();

    mm_zones_init(arg);

    (void)mm_heap_init();
    (void)mm_slab_init();

    kprint("mmu: memory allocators initialized\n");

    return 0;
}
