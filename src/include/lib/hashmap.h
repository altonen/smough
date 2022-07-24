#ifndef __HASHMAP_H__
#define __HASHMAP_H__

#include <stddef.h>
#include <stdint.h>

typedef enum {
    HM_KEY_TYPE_NUM = 0,
    HM_KEY_TYPE_STR = 1,
} hm_key_type_t;

typedef struct hashmap hashmap_t;

hashmap_t *hm_alloc_hashmap(size_t size, hm_key_type_t type);
void       hm_dealloc_hashmap(hashmap_t *hm);

int hm_insert(hashmap_t *hm, void *ukey, void *elem);
int hm_remove(hashmap_t *hm, void *ukey);

void   *hm_get(hashmap_t *hm, void *ukey);
size_t  hm_get_size(hashmap_t *hm);
size_t  hm_get_capacity(hashmap_t *hm);

void hm_add_hash_func(hashmap_t *hm, uint32_t (*hm_hash_func)(void *));

#endif /* __HASHMAP_H__ */
