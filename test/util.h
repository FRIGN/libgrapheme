/* See LICENSE file for copyright and license details. */
#ifndef UTIL_H
#define UTIL_H

#include "../gen/types.h"
#include "../grapheme.h"

#undef MIN
#define MIN(x,y)  ((x) < (y) ? (x) : (y))
#undef LEN
#define LEN(x) (sizeof(x) / sizeof(*(x)))

int run_break_tests(size_t (*next_break)(const uint_least32_t *, size_t),
                    const struct break_test *test, size_t testlen,
                    const char *);
int run_unit_tests(int (*unit_test_callback)(void *, size_t, const char *,
                   const char *), void *, size_t, const char *, const char *);

#endif /* UTIL_H */
