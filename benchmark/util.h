/* See LICENSE file for copyright and license details. */
#ifndef UTIL_H
#define UTIL_H

#include "../gen/types.h"

#define LEN(x) (sizeof(x) / sizeof(*(x)))

uint32_t *generate_test_buffer(const struct test *, size_t, size_t *);
void run_benchmark(void (*func)(const void *), const void *, const char *,
                   double *, uint32_t);

#endif /* UTIL_H */
