#include <kernel/util.h>
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
