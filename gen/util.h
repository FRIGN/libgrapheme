/* See LICENSE file for copyright and license details. */
#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>
#include <stdint.h>

#define LEN(x) (sizeof (x) / sizeof *(x))

struct property_spec {
	const char *enumname;
	const char *file;
	const char *ucdname;
};

struct properties {
	uint_least8_t break_property;
};

struct segment_test {
	uint_least32_t *cp;
	size_t cplen;
	size_t *len;
	size_t lenlen;
	char *descr;
};

void parse_file_with_callback(const char *, int (*callback)(const char *,
                              char **, size_t, char *, void *), void *payload);

void properties_generate_break_property(const struct property_spec *,
                                        uint_least8_t, const char *,
                                        const char *);

void segment_test_list_parse(char *, struct segment_test **, size_t *);
void segment_test_list_print(const struct segment_test *, size_t,
                             const char *, const char *);
void segment_test_list_free(struct segment_test *, size_t);

#endif /* UTIL_H */
