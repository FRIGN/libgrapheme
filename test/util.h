/* See LICENSE file for copyright and license details. */
#ifndef UTIL_H
#define UTIL_H

#include "../gen/types.h"
#include "../grapheme.h"

#define LEN(x) (sizeof(x) / sizeof(*(x)))

int run_break_tests(bool (*is_break)(uint_least32_t, uint_least32_t,
                    GRAPHEME_STATE *), const struct break_test *test,
                    size_t testlen, const char *);

#endif /* UTIL_H */
