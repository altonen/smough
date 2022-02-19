#include <fs/elf.h>
#include <fs/multiboot2.h>
#include <kernel/common.h>
#include <mm/mmu.h>
#include <sys/types.h>

size_t multiboot2_map_memory(uint64_t *address, void (*callback)(uint32_t, uint64_t, size_t))
{
    struct multiboot_tag *tag;
    multiboot_memory_map_t *mmap;

    for (tag = (struct multiboot_tag *)(address + 8);
         tag->type != MULTIBOOT_TAG_TYPE_END;
         tag = (struct multiboot_tag *)((multiboot_uint8_t *)tag + ((tag->size + 7) & ~7)))
    {
        switch (tag->type) {
            case MULTIBOOT_TAG_TYPE_MMAP: {
                mmap = ((multiboot_tag_mmap_t *)tag)->entries;

                do {
                    uint64_t addr = (mmap->addr >> 32) | (mmap->addr & 0xffffffff);
                    uint64_t len  = (mmap->len  >> 32) | (mmap->len  & 0xffffffff);

                    switch (mmap->type) {
                        case MULTIBOOT_MEMORY_AVAILABLE:
                        case MULTIBOOT_MEMORY_RESERVED:
                        case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
                        case MULTIBOOT_MEMORY_NVS:
                        case MULTIBOOT_MEMORY_BADRAM:
                            callback(mmap->type, addr, len);
                        break;
                    }

                    mmap = (multiboot_memory_map_t *)((uint64_t)mmap + ((multiboot_tag_mmap_t *)tag)->entry_size);
                } while ((multiboot_uint8_t *)mmap < (multiboot_uint8_t *)tag + tag->size);
            }
            break;
        }
    }

    return 0;
}
