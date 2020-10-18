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

void parse_input(int (*process_line)(char **, size_t, char *));
int cp_parse(const char *, uint32_t *);
int range_parse(const char *, struct range *);
void range_list_append(struct range **, size_t *, const struct range *);

#endif /* UTIL_H */
