#include <lib/bitmap.h>
#include <kernel/kprint.h>
#include <errno.h>

#define BM_GET_MULTIPLE_OF_32(n) (n % 32) ? ((n / 32) + 1) : (n / 32)

int bm_set_bit(bitmap_t *bm, uint32_t n)
{
    if (!bm)
        return -EINVAL;

    if (n / 32 > bm->len)
        return -E2BIG;

    bm->bits[n / 32] |= 1 << (n % 32);
    return 0;
}

int bm_set_range(bitmap_t *bm, uint32_t n, uint32_t k)
{
    if (n >= bm->len || k >= bm->len)
        return -E2BIG;

    while (n <= k) {
        bm->bits[n / 32] |= 1 << (n % 32);
        n++;
    }

    return 0;
}

int bm_unset_bit(bitmap_t *bm, uint32_t n)
{
    if (n >= bm->len)
        return -E2BIG;

    bm->bits[n / 32] &= ~(1 << (n % 32));
    return 0;
}

int bm_unset_range(bitmap_t *bm, uint32_t n, uint32_t k)
{
    if (n >= bm->len || k >= bm->len)
        return -E2BIG;

    while (n <= k) {
        bm->bits[n / 32] &= ~(1 << (n % 32));
        n++;
    }

    return 0;
}

int bm_test_bit(bitmap_t *bm, uint32_t n)
{
    if (n >= bm->len)
        return -E2BIG;

    return (bm->bits[n / 32] & (1 << (n % 32))) >> (n % 32);
}

static int bm_find_first(bitmap_t *bm, uint32_t n, uint32_t k, uint8_t bit_status)
{
    if (n >= bm->len || k >= bm->len)
        return -E2BIG;

    while (n <= k) {
        if (((bm->bits[n / 32] & (1 << (n % 32))) >> (n % 32)) == bit_status)
            return n;
        n++;
    }

    return -ENOENT;
}

int bm_find_first_unset(bitmap_t *bm, uint32_t n, uint32_t k)
{
    return bm_find_first(bm, n, k, 0);
}

int bm_find_first_set(bitmap_t *bm, uint32_t n, uint32_t k)
{
    return bm_find_first(bm, n, k, 1);
}

static int bm_find_first_range(bitmap_t *bm, uint32_t n, uint32_t k, size_t len, uint8_t bit_status)
{
    if (n >= bm->len || k >= bm->len)
        return -E2BIG;

    int start = -ENOENT;
    size_t cur_len = 0;

    while (n <= k) {
        if (((bm->bits[n / 32] & (1 << (n % 32))) >> (n % 32)) == bit_status) {

            if (start == -ENOENT)
                start = n;

            if (++cur_len == len)
                return start;

        } else {
            start   = -ENOENT;
            cur_len = 0;
        }
        n++;
    }

    return -ENOENT;
}

int bm_find_first_set_range(bitmap_t *bm, uint32_t n, uint32_t k, size_t len)
{
    return bm_find_first_range(bm, n, k, len, 1);
}

int bm_find_first_unset_range(bitmap_t *bm, uint32_t n, uint32_t k, size_t len)
{
    return bm_find_first_range(bm, n, k, len, 0);
}
