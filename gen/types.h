/* See LICENSE file for copyright and license details. */
#ifndef TYPES_H
#define TYPES_H

#include <stddef.h>
#include <stdint.h>

struct range {
	uint_least32_t lower;
	uint_least32_t upper;
};

struct range_list {
	struct range *data;
	size_t len;
};

struct test {
	uint_least32_t *cp;
	size_t cplen;
	size_t *len;
	size_t lenlen;
	char *descr;
};

#endif /* TYPES_H */
