#ifndef __LIST_H__
#define __LIST_H__

#include <stddef.h>

typedef struct list_head {
	struct list_head *next;
	struct list_head *prev;
} list_head_t;

#define FOREACH(list, __i) \
    for (list_head_t *__i = list.next; __i != &list; __i = __i->next)

#define container_of(ptr, type, member) ({ \
                const typeof(((type *)0)->member) *__mptr = (ptr); \
                (type *)((char *)__mptr - offsetof(type, member));})

void list_remove(list_head_t *s);
void list_init(list_head_t *s);
void list_init_null(list_head_t *s);

void list_append(list_head_t *s, list_head_t *t);
void list_prepend(list_head_t *s, list_head_t *t);

void list_insert(list_head_t *new, list_head_t *next, list_head_t *prev);
void list_insert_tail(list_head_t *s, list_head_t *t);
void list_insert_head(list_head_t *s, list_head_t *t);

#endif /* __LIST_H__ */
