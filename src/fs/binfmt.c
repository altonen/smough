#include <fs/binfmt.h>
#include <kernel/kpanic.h>
#include <mm/heap.h>

static list_head_t loaders;

typedef struct {
    binfmt_loader_t loader;
    list_head_t list;
} binfmt_t;

void binfmt_init(void)
{
    list_init(&loaders);
}

void binfmt_add_loader(binfmt_loader_t loader)
{
    binfmt_t *bfmt = kmalloc(sizeof(binfmt_t));
    bfmt->loader = loader;

    list_init(&bfmt->list);
    list_append(&loaders, &bfmt->list);
}

bool binfmt_load(file_t *file, int argc, char **argv)
{
    list_head_t *iter = NULL;
    binfmt_t *loader  = NULL;

    FOREACH(loaders, l) {
        loader = container_of(l, binfmt_t, list);

        if (loader->loader && loader->loader(file, argc, argv))
            kpanic("binfmt load_bin returned!");
    }

    return false;
}
