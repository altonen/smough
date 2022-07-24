#include <fs/fs.h>
#include <fs/dentry.h>
#include <kernel/kpanic.h>
#include <kernel/kprint.h>
#include <kernel/util.h>
#include <lib/hashmap.h>
#include <lib/list.h>
#include <mm/slab.h>
#include <errno.h>
#include <stdbool.h>

#define DENTRY_HM_LEN 32
#define USE_CACHE(flags) (!((bool)(flags & DNTR_NO_CACHE)))

static mm_cache_t *dentry_cache = NULL;

int dentry_init(void)
{
    if (!(dentry_cache = mm_cache_create(sizeof(dentry_t))))
        kpanic("failed to initialize slab cache for dentries!");

    return 0;
}

static dentry_t *__dentry_alloc(char *name, bool cache)
{
    (void)name;

    dentry_t *dntr = NULL;

    if (!(dntr = mm_cache_alloc_entry(dentry_cache)))
        return NULL;

    // don't cache this dentry to global dentry cache (used for . and ..)
    if (!cache)
        return dntr;

    return dntr;
}

// alloc and initialize empty dentry
static dentry_t *__dentry_alloc_empty(dentry_t *parent, char *name, bool cache)
{
    dentry_t *dntr = NULL;
    size_t len     = kstrlen(name) + 1;

    if (!parent || !name) {
        errno = EINVAL;
        return NULL;
    }

    if (len > DENTRY_NAME_MAXLEN) {
        errno = E2BIG;
        return NULL;
    }

    if (hm_get(parent->d_children, name)) {
        errno = EEXIST;
        return NULL;
    }

    if (!(dntr = __dentry_alloc(name, cache))) {
        errno = ENOMEM;
        return NULL;
    }

    dntr->d_parent   = parent;
    dntr->d_private  = NULL;
    dntr->d_inode    = NULL;
    dntr->d_flags    = 0;
    dntr->d_count    = 1;

    kstrncpy(dntr->d_name, name, len);
    list_init(&dntr->d_list);

    // update parent
    if ((errno = hm_insert(parent->d_children, dntr->d_name, dntr)) < 0) {
        // TODO: release allocated dentry
        return NULL;
    }

    parent->d_count++;
    list_append(&parent->d_list, &dntr->d_list);

    return dntr;
}

static int __dentry_init_children(dentry_t *parent, dentry_t *dntr, uint32_t flags)
{
    dentry_t *this = NULL,
             *prnt = NULL;

    if (!(dntr->d_children = hm_alloc_hashmap(DENTRY_HM_LEN, HM_KEY_TYPE_STR)))
        goto error;

    if (!(this = __dentry_alloc_empty(dntr, ".", false)))
        goto error;

    this->d_parent = NULL;
    this->d_flags  = T_IFREG;

    if (!(prnt = __dentry_alloc_empty(dntr, "..", false)))
        goto error_dealloc;

    prnt->d_parent = parent;
    prnt->d_flags  = T_IFREG | flags;

    if (parent)
        prnt->d_inode = parent->d_inode;

    return 0;

error_dealloc:
    (void)dentry_dealloc(this);

error:
    return -errno;
}

dentry_t *dentry_alloc(dentry_t *parent, char *name, uint32_t flags)
{
    int ret        = 0;
    dentry_t *dntr = NULL;

    if (!parent) {
        errno = EINVAL;
        return NULL;
    }

    if (hm_get(parent->d_children, name)) {
        errno = EEXIST;
        return NULL;
    }

    if (!(dntr = __dentry_alloc_empty(parent, name, USE_CACHE(flags)))) {
        errno = ENOMEM;
        return NULL;
    }

    if (((dntr->d_flags = flags) & T_IFDIR)) {
        if ((ret = __dentry_init_children(parent, dntr, flags)) < 0) {
            if (dentry_dealloc(dntr))
                kprint("dentry - failed to deallocate dentry!\n");

            errno = -ret;
            return NULL;
        }
    }

    return dntr;
}

dentry_t *dentry_alloc_orphan(char *name, uint32_t flags)
{
    int ret        = 0;
    dentry_t *dntr = NULL;

    if (!(dntr = __dentry_alloc(name, USE_CACHE(flags)))) {
        errno = ENOMEM;
        return NULL;
    }

    dntr->d_count  = 1;
    dntr->d_flags  = flags;
    dntr->d_parent = NULL;
    dntr->d_inode  = NULL;

    kstrncpy(dntr->d_name, name, kstrlen(name) + 1);
    list_init(&dntr->d_list);

    if (flags & T_IFDIR) {
        if ((ret = __dentry_init_children(NULL, dntr, flags)) < 0) {
            if (dentry_dealloc(dntr) < 0)
                kprint("dentry - failed to deallocate dentry!\n");

            errno = -ret;
            return NULL;
        }
    }

    return dntr;
}

dentry_t *dentry_alloc_ino(dentry_t *parent, char *name, inode_t *ino, uint32_t flags)
{
    if (!ino) {
        errno = EINVAL;
        return NULL;
    }

    dentry_t *dntr = NULL;

    if ((dntr = dentry_alloc(parent, name, flags)))
        dntr->d_inode = ino;

    ino->i_count++;
    return dntr;
}

dentry_t *dentry_alloc_orphan_ino(char *name, inode_t *ino, uint32_t flags)
{
    if (!ino) {
        errno = EINVAL;
        return NULL;
    }

    dentry_t *dntr = NULL;

    if ((dntr = dentry_alloc_orphan(name, flags)))
        dntr->d_inode = ino;

    ino->i_count++;
    return dntr;
}

int dentry_dealloc(dentry_t *dntr)
{
    int ret = 0;

    if (!dntr)
        return -EINVAL;

    if (dntr->d_count > 1)
        return -EBUSY;

    if (dntr->d_parent) {
        if ((ret = hm_remove(dntr->d_parent->d_children, dntr->d_name)) < 0)
            return ret;
    }

    if (dntr->d_inode)
        dntr->d_inode->i_count--;

    list_remove(&dntr->d_list);
    hm_dealloc_hashmap(dntr->d_children);
    (void)mm_cache_free_entry(dentry_cache, dntr);

    return ret;
}

// TODO: global dentry cache

dentry_t *dentry_cache_lookup(char *name)
{
    (void)name;

    return NULL;
}

dentry_t *dentry_cache_insert(dentry_t *dntr)
{
    (void)dntr;

    return NULL;
}

int dentry_cache_evict(dentry_t *dntr)
{
    (void)dntr;

    return 0;
}
