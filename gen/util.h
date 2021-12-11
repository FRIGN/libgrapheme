/* See LICENSE file for copyright and license details. */
#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>
#include <stdint.h>

#define LEN(x) (sizeof (x) / sizeof *(x))

struct range {
	uint_least32_t lower;
	uint_least32_t upper;
};

struct property {
	char         *enumname;
	char         *identifier;
	char         *fname;
	struct range *table;
	size_t        tablelen;
};

struct segment_test {
	uint_least32_t *cp;
	size_t cplen;
	size_t *len;
	size_t lenlen;
	char *descr;
};

void property_list_parse(struct property *, size_t);
void property_list_print(const struct property *, size_t, const char *, const char *);

void segment_test_list_parse(char *, struct segment_test **, size_t *);
void segment_test_list_print(struct segment_test *, size_t, const char *, const char *);

#endif /* UTIL_H */
