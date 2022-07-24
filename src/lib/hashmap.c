#include <kernel/kassert.h>
#include <kernel/util.h>
#include <lib/hashmap.h>
#include <mm/heap.h>

#include <errno.h>
#include <stdbool.h>

#define BUCKET_MAX_LEN 12
#define KEY_SIZE       12

typedef struct hm_item {
    void *data;
    uint32_t key;
    bool occupied;
} hm_item_t;

struct hashmap {
    size_t len;
    size_t cap;

    hm_item_t **elem;
    uint32_t (*hm_hash)(void *);
};

/*  https://stackoverflow.com/questions/664014/what-integer-
 *  hash-function-are-good-that-accepts-an-integer-hash-key */
static uint32_t hm_hash_num(void *key)
{
    if (!key)
        return UINT32_MAX;

    uint32_t tmp = *(uint32_t *)key;

    tmp = ((tmp >> 16) ^ tmp) * 0x45d9f3b;
    tmp = ((tmp >> 16) ^ tmp) * 0x45d9f3b;
    tmp = ((tmp >> 16) ^ tmp);

    return tmp;
}

/*  http://www.cse.yorku.ca/~oz/hash.html */
static uint32_t hm_hash_str(void *key)
{
    if (!key)
        return UINT32_MAX;

    uint32_t hash = 5381;
    char *str     = key;
    int c;

    while ((c = *str++) != '\0')
        hash = ((hash << 5) + hash) + c;

    return hash;
}

/* "size" should be large enough so that rehashing
 * needn't to be performed as it's very expensive */
hashmap_t *hm_alloc_hashmap(size_t size, hm_key_type_t type)
{
    hashmap_t *hm;

    if (!(hm = kmalloc(sizeof(hashmap_t))))
        return NULL;

    if (!(hm->elem = kzalloc(size * sizeof(hm_item_t *))))
        return NULL;

    hm->len  = 0;
    hm->cap  = size;

	switch (type) {
        case HM_KEY_TYPE_NUM:
            hm->hm_hash = hm_hash_num;
            return hm;
        case HM_KEY_TYPE_STR:
            hm->hm_hash = hm_hash_str;
            return hm;
        default:
            errno = EINVAL;
            return NULL;
    }

    return hm;
}

void hm_dealloc_hashmap(hashmap_t *hm)
{
    if (!hm)
        return;

    if (!hm->elem)
        goto free_hm;

    for (size_t i = 0; i < hm->len; ++i)
        kfree(hm->elem[i]);

    kfree(hm->elem);

free_hm:
    kfree(hm);
}

static int hm_find_free_bucket(hashmap_t *hm, uint32_t key)
{
    int index = key % hm->cap;

    for (size_t i = 0; i < BUCKET_MAX_LEN; ++i) {
        if (!hm->elem[index]) {
            hm->elem[index] = kmalloc(sizeof(hm_item_t));
            return index;
        }

        if (!hm->elem[index]->occupied)
            return index;

        index = (index + 1) % hm->cap;
    }

    return -ENOSPC;
}

int hm_insert(hashmap_t *hm, void *ukey, void *elem)
{
    kassert(hm != NULL && hm->elem != 0);
    kassert(hm->len < hm->cap); // TODO: fix

    uint32_t key;
    int index;

    if ((key = hm->hm_hash(ukey)) == UINT32_MAX)
        return -EINVAL;

    index = hm_find_free_bucket(hm, key);

    if (index < 0) {
        return -ENOSPC;
    }

    hm->elem[index]->data     = elem;
    hm->elem[index]->key      = key;
    hm->elem[index]->occupied = true;
    hm->len++;

    return 0;
}

int hm_remove(hashmap_t *hm, void *ukey)
{
    kassert(hm != NULL);

    uint32_t key;
    int index;

    if ((key = hm->hm_hash(ukey)) == UINT32_MAX)
        return -EINVAL;

    index = key % hm->cap;

    for (size_t i = 0; i < BUCKET_MAX_LEN; ++i) {
        if (hm->elem[index]->key == key) {
            hm->elem[index]->occupied = false;
            return 0;
        }

        index = (index + 1) % hm->cap;
    }

    return -ENOENT;
}

void *hm_get(hashmap_t *hm, void *ukey)
{
    kassert(hm != NULL && hm->elem != 0);

    uint32_t key;
    int index;

    if ((key = hm->hm_hash(ukey)) == UINT32_MAX)
        return NULL;

    index = key % hm->cap;

    for (size_t i = 0; i < BUCKET_MAX_LEN; ++i) {
        if (!hm->elem[index])
            return NULL;
        if (!hm->elem[index]->occupied)
            return NULL;
        if (hm->elem[index]->key == key)
            return hm->elem[index]->data;

        index = (index + 1) % hm->cap;
    }

    return NULL;
}

size_t hm_get_size(hashmap_t *hm)
{
    return hm ? hm->len : 0;
}

size_t hm_get_capacity(hashmap_t *hm)
{
    return hm ? hm->cap : 0;
}

void hm_add_hash_func(hashmap_t *hm, uint32_t (*hm_hash_func)(void *))
{
    kassert(hm != NULL && hm_hash_func != NULL);
    hm->hm_hash = hm_hash_func;
}
