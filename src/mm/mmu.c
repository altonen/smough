#include <arch/amd64/mmu.h>
#include <mm/mmu.h>

int mmu_init(void)
{
    /* initialize the native memory management unit
     * and whatever else the native platform needs */
    (void)mmu_native_init();

    return 0;
}
