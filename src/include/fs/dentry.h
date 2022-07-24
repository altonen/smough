#ifndef __DENTRY_H__
#define __DENTRY_H__

#include <fs/inode.h>
#include <lib/list.h>
#include <lib/hashmap.h>
#include <stdint.h>

#define DENTRY_NAME_MAXLEN 128

typedef struct inode      inode_t;
typedef struct dentry     dentry_t;
typedef struct dentry_ops dentry_ops_t;

enum DENTRY_FLAGS {
    DNTR_NO_CACHE = 1 << 7,
};

struct dentry {
    uint32_t  d_flags;

    dentry_t *d_parent;
    inode_t  *d_inode;
    hashmap_t *d_children;

    int d_count;
    list_head_t d_list;
    void *d_private;

    char d_name[DENTRY_NAME_MAXLEN];
};

int dentry_init(void);

dentry_t *dentry_alloc(dentry_t *parent, char *name, uint32_t flags);
dentry_t *dentry_alloc_orphan(char *name, uint32_t flags);
dentry_t *dentry_alloc_ino(dentry_t *parent, char *name, inode_t *ino, uint32_t flags);
dentry_t *dentry_alloc_orphan_ino(char *name, inode_t *ino, uint32_t flags);
int       dentry_dealloc(dentry_t *dntr);

dentry_t *dentry_lookup(dentry_t *parent, char *name);

dentry_t *dentry_cache_lookup(char *name);
dentry_t *dentry_cache_insert(dentry_t *dntr);
int       dentry_cache_evict(dentry_t *dntr);

#endif /* __DENTRY_H__ */
