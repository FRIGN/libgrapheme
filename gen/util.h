/* See LICENSE file for copyright and license details. */
#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>
#include <stdint.h>

#include "types.h"

#define LEN(x) (sizeof (x) / sizeof *(x))

struct property_spec {
	const char *enumname;
	const char *file;
	const char *ucdname;
};

struct properties {
	uint_least8_t break_property;
};

void parse_file_with_callback(const char *, int (*callback)(const char *,
                              char **, size_t, char *, void *), void *payload);

void properties_generate_break_property(const struct property_spec *,
                                        uint_least8_t, const char *,
                                        const char *);

void break_test_list_parse(char *, struct break_test **, size_t *);
void break_test_list_print(const struct break_test *, size_t,
                             const char *, const char *);
void break_test_list_free(struct break_test *, size_t);

#endif /* UTIL_H */
