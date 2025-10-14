/* See LICENSE file for copyright and license details. */
#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>
#include <stdint.h>

#define LEN(x) (sizeof(x) / sizeof *(x))

struct codepoint_property {
	char **fields;
	size_t field_count;
};

struct codepoint_property_set {
	struct codepoint_property *properties;
	size_t property_count;
};

struct codepoint_property_set *parse_property_file(const char *);
void free_codepoint_property_set_array(struct codepoint_property_set *);
const struct codepoint_property *match_in_codepoint_property_set(
	const struct codepoint_property_set *, const char *, size_t);
void compress_and_output(uint_least64_t *, const char *);

#endif /* UTIL_H */
