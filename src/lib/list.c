#include <lib/list.h>
#include <kernel/kprint.h>

void list_init(list_head_t *s)
{
    s->next = s;
    s->prev = s;
}

void list_init_null(list_head_t *s)
{
    s->next = NULL;
    s->prev = NULL;
}

void list_remove(list_head_t *s)
{
    if (s->next)
        s->next->prev = s->prev;
    if (s->prev)
        s->prev->next = s->next;
}

void list_insert(list_head_t *new, list_head_t *next, list_head_t *prev)
{
    prev->next = new;
    next->prev = new;
    new->prev = prev;
    new->next = next;
}

void list_append(list_head_t *s, list_head_t *t)
{
    t->next = s->next;
    t->prev = s;

    if (s->next)
        s->next->prev = t;

    s->next = t;
}

void list_insert_tail(list_head_t *s, list_head_t *t)
{
    s->next = t;
    t->prev = s;
    t->next = NULL;
}

void list_insert_head(list_head_t *s, list_head_t *t)
{
    s->prev = t;
    t->next = s;
    t->prev = NULL;
}

void list_prepend(list_head_t *s, list_head_t *t)
{
    t->prev = s->prev;
    t->next = s;
    s->prev = t;

    if (s->prev)
        s->prev->next = t;
}
