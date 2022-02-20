#include <arch/amd64/mmu.h>
#include <mm/bootmem.h>
#include <mm/heap.h>
#include <mm/mmu.h>

int mm_init(void *arg)
{
    /* initialize the native memory management unit
     * and whatever else the native platform needs */
    (void)mm_native_init();

    (void)mm_bootmem_init(arg);
    (void)mm_heap_preinit();

    return 0;
}
