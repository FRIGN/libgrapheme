/* See LICENSE file for copyright and license details. */
#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>
#include <stdint.h>

#include "../gen/types.h"
#include "../grapheme.h"

#define LEN(x) (sizeof(x) / sizeof(*(x)))

int heisenstate_get(struct grapheme_internal_heisenstate *, int);
int heisenstate_set(struct grapheme_internal_heisenstate *, int, int);

int has_property(uint_least32_t, struct grapheme_internal_heisenstate *,
                 const struct range_list *, int);

#endif /* UTIL_H */
