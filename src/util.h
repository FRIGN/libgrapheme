/* See LICENSE file for copyright and license details. */
#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>
#include <stdint.h>

#include "../grapheme.h"

#define LEN(x) (sizeof(x) / sizeof(*(x)))

struct range {
	uint32_t lower;
	uint32_t upper;
};

struct range_list {
	struct range *data;
	size_t len;
};

int heisenstate_get(struct lg_internal_heisenstate *, int);
int heisenstate_set(struct lg_internal_heisenstate *, int, int);

int has_property(uint32_t, struct lg_internal_heisenstate *,
                 const struct range_list *, int);

#endif /* UTIL_H */
