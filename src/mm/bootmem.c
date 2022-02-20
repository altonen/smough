#include <fs/multiboot2.h>
#include <lib/bitmap.h>
#include <kernel/common.h>
#include <mm/types.h>
#include <mm/bootmem.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>

#define BOOTMEM_MAX_RANGES     5
#define BOOTMEM_MAX_ENTRIES  256

/* Boot memory initialization procedure
 *
 * Before smough is usable for normal operation, its memory allocators need to be initialized
 * but these memory allocators themselves also allocate memory when they are initialized so
 * it's a bit of a catch-22 problem.
 *
 * To remedy this situation, smough initializes so called "boot memory" which is used only
 * during booting and only for creating a temporary memory allocator which can be used to
 * initializes the other memory allocators.
 *
 * The boot memory is represented as a simple bitmap of free pages. This information is given
 * to smough by Multiboot2 which provides us a map of available and reclaimable memory ranges
 * which can then be used for temporary memory allocation.
 *
 * The memory map given by Multiboot2 can be split into several ranges and for now, smough only
 * maps the first five ranges, each of which can contain up to 256 * 32 * 4096 bytes (32 MB) of
 * memory which is plenty enough for booting.
 *
 * Because at this point in booting smough doesn't have access to heap, the bitmap cannot be allocated
 * "the correct way" but its memory is manually initialized to point to an array of uint32_ts */

struct {
    struct {
        unsigned long start;
        size_t len;
        uint32_t mem[BOOTMEM_MAX_ENTRIES];
        bitmap_t bm;
    } ranges[BOOTMEM_MAX_RANGES];

    size_t ptr;
} mem_info;

static void bootmem_claim_range(unsigned type, unsigned long addr, size_t len)
{
    if (type != MULTIBOOT_MEMORY_AVAILABLE &&
        type != MULTIBOOT_MEMORY_ACPI_RECLAIMABLE)
        return;

    if (mem_info.ptr >= BOOTMEM_MAX_RANGES)
        return;

	/* discard ranges that contain less than a page of memory */
    size_t npages = ROUND_DOWN(len, PAGE_SIZE) / PAGE_SIZE;
    if (npages == 0)
        return;

    kprint("bootmem: free range 0x%x - 0x%x (%u, %u)\n", addr, addr + len, len, npages);

    mem_info.ranges[mem_info.ptr].start = ROUND_UP(addr, PAGE_SIZE);
    bm_unset_range(&mem_info.ranges[mem_info.ptr].bm, 0, npages);

    mem_info.ptr++;
}

int mm_bootmem_init(void *arg)
{
    kprint("bootmem: initialize boot memory maps");

    for (int i = 0; i < BOOTMEM_MAX_RANGES; ++i) {
        mem_info.ranges[i].bm.bits = mem_info.ranges[i].mem;
        mem_info.ranges[i].start   = INVALID_ADDRESS;
        mem_info.ranges[i].bm.len  = BOOTMEM_MAX_ENTRIES * 32;

        bm_set_range(&mem_info.ranges[i].bm, 0, BOOTMEM_MAX_ENTRIES * 32 - 1);
    }

    mem_info.ptr = 0;
    multiboot2_map_memory(arg, bootmem_claim_range);

    return 0;
}

uint64_t mm_bootmem_alloc_block(size_t npages)
{
    for (size_t i = 0; i < mem_info.ptr; ++i) {
        int start = bm_find_first_unset_range(
            &mem_info.ranges[i].bm,
            0,
            mem_info.ranges[i].bm.len - 1,
            npages
        );

        if (start == -ENOENT)
            continue;

        if (npages == 1) {
            if (bm_set_bit(&mem_info.ranges[i].bm, start) != 0)
                return INVALID_ADDRESS;
        } else if (bm_set_range(&mem_info.ranges[i].bm, start, start + npages) != 0)
            return INVALID_ADDRESS;

        return mem_info.ranges[i].start + PAGE_SIZE * start;
    }

    return INVALID_ADDRESS;
}
