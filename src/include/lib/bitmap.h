#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <stdint.h>
#include <stddef.h>

typedef struct bitmap {
    size_t len;
    uint32_t *bits;
} bitmap_t;

#define TO_BM_LEN(n)    ((n) / 32)
#define BM_GET_SIZE(bm) (sizeof(*bm) + sizeof(uint32_t) * (bm->len / 32))

/* set bit
 *
 * return 0 sucess
 * return -EINVAL if `bm` is NULL
 * return -E2BIG if `n` out of range */
int bm_set_bit(bitmap_t *bm, uint32_t n);

/* set a range of bits
 *
 * return 0 sucess
 * return -EINVAL if `bm` is NULL
 * return -E2BIG if `k` out of range */
int bm_set_range(bitmap_t *bm, uint32_t n, uint32_t k);

/* unset bit
 *
 * return 0 sucess
 * return -EINVAL if `bm` is NULL
 * return -E2BIG if `n` out of range */
int bm_unset_bit(bitmap_t *bm, uint32_t n);

/* unset a range of bits
 *
 * return 0 sucess
 * return -EINVAL if `bm` is NULL
 * return -E2BIG if `k` out of range */
int bm_unset_range(bitmap_t *bm, uint32_t n, uint32_t k);

/* check if a bit is set
 *
 * return 1 if the bit is set
 * return 0 if the bit is not set
 * return -EINVAL if `bm` is NULL
 * return -E2BIG if `n` out of range */
int bm_test_bit(bitmap_t *bm, uint32_t n);

/* find first set bit from range `[n, k]`
 *
 * return the position (`n <= x <= k`) if a bit is set
 * return -ENOENT if all bits in the range are unset
 * return -EINVAL if `bm` is NULL
 * return -E2BIG if `n`, `k` out of range */
int bm_find_first_set(bitmap_t *bm, uint32_t n, uint32_t k);

/* find first unset bit from range `[n, k]`
 *
 * return the position (`n <= x <= k`) if a bit is set
 * return -ENOENT if all bits in the range are set
 * return -EINVAL if `bm` is NULL
 * return -E2BIG if `n`, `k` out of range */
int bm_find_first_unset(bitmap_t *bm, uint32_t n, uint32_t k);

/* find first range with all bits set to 1 that is at least `len` bits long
 *
 * return start of the set range
 * return -ENOENT if all bits in the range are unset
 * return -EINVAL if `bm` is NULL
 * return -E2BIG if `n`, `k` out of range */
int bm_find_first_set_range(bitmap_t *bm, uint32_t n, uint32_t k, size_t len);

/* find first range with all bits set to 0 that is at least `len` bits long
 *
 * return start of the set range
 * return -ENOENT if all bits in the range are unset
 * return -EINVAL if `bm` is NULL
 * return -E2BIG if `n`, `k` out of range */
int bm_find_first_unset_range(bitmap_t *bm, uint32_t n, uint32_t k, size_t len);

#endif /* __BITMAP_H__ */
