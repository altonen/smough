#include <kernel/util.h>
#include <mm/heap.h>
#include <stdint.h>

size_t kstrlen(const char *str)
{
    size_t len = 0;

    while (str[len] != '\0')
        len++;

    return len;
}

void *kmemcpy(void *restrict dstptr, const void *restrict srcptr, size_t size)
{
    unsigned char *dst = dstptr;
    const unsigned char *src = srcptr;

    for (size_t i = 0; i < size; ++i)
        dst[i] = src[i];

    return dstptr;
}

void *kmemset(void *buf, int c, size_t size)
{
    unsigned char *ptr = buf;

    while (size--)
        *ptr++ = c;

    return buf;
}

int kmemcmp(void *s1, void *s2, size_t n)
{
    uint8_t *s1_ptr = s1;
    uint8_t *s2_ptr = s2;

    for (size_t i = 0; i < n; ++i) {
        if (s1_ptr[i] < s2_ptr[i])
            return -1;

        if (s1_ptr[i] > s2_ptr[i])
            return 1;
    }

    return 0;
}

int kstrcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2))
        s1++, s2++;

    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

int kstrncmp(const char *s1, const char *s2, size_t len)
{
    size_t i;

    for (i = 0; s1[i] != '\0' && s2[i] != '\0' && i < len; ++i) {
        if (s1[i] < s2[i])
            return -1;
        else if (s1[i] > s2[i])
            return 1;
    }

    return !(s1[i] == s2[i]);
}

char *kstrcpy(char *dst, char *src)
{
    for (size_t i = 0; src[i] != '\0'; ++i)
        dst[i] = src[i];

    return dst;
}

char *kstrncpy(char *restrict dst, const char *restrict src, size_t n)
{
    size_t i;

    for (i = 0; i < n && src[i] != '\0'; ++i)
        dst[i] = src[i];
    for (; i < n; ++i)
        dst[i] = '\0';

    return dst;
}

char *kstrncat(char *s1, char *s2, size_t len)
{
    char *ret = NULL, *rptr = NULL;
    char *ptr = s1;

    ret = rptr = kmalloc(len + 1);

    while (*ptr != '\0')
        *rptr = *ptr, rptr++, ptr++;

    ptr = s2;
    while (*ptr != '\0')
        *rptr = *ptr, rptr++, ptr++;

    return ret;
}

char *kstrcat_s(char *s1, char *s2)
{
    size_t len = kstrlen(s1) + kstrlen(s2);

    return kstrncat(s1, s2, len);
}

char *kstrdup(const char *s)
{
    size_t n  = kstrlen(s), i;
    char *new = kmalloc(n + 1);

    if (!new)
        return NULL;

    for (i = 0; i < n; ++i)
        new[i] = s[i];
    new[i] = '\0';

    return new;
}

int kstrcmp_s(const char *s1, const char *s2)
{
    size_t len = kstrlen(s1);
    return kstrncmp(s1, s2, len);
}
