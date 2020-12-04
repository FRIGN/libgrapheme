/* See LICENSE file for copyright and license details. */
#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>
#include <stdint.h>

#define LEN(x) (sizeof (x) / sizeof *(x))

struct range {
	uint32_t lower;
	uint32_t upper;
};

struct range_list {
	struct range *data;
	size_t len;
};

/* 16-slot (0,...,15) optionally undetermined binary state */
struct heisenstate {
	uint_least16_t determined;
	uint_least16_t state;
};

int heisenstate_get(struct heisenstate *, int);
int heisenstate_set(struct heisenstate *, int, int);

#endif /* UTIL_H */
