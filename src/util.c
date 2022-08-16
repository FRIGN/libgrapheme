/* See LICENSE file for copyright and license details. */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "../gen/types.h"
#include "../grapheme.h"
#include "util.h"

inline size_t
get_codepoint(const void *str, size_t len, size_t offset, uint_least32_t *cp)
{
	if (offset < len) {
		*cp = ((const uint_least32_t *)str)[offset];
		return 1;
	} else {
		*cp = GRAPHEME_INVALID_CODEPOINT;
		return 0;
	}
}

inline size_t
get_codepoint_utf8(const void *str, size_t len, size_t offset, uint_least32_t *cp)
{
	size_t ret;

	if (offset < len) {
		ret = grapheme_decode_utf8((const char *)str + offset,
		                           len - offset, cp);

		if (unlikely(len == SIZE_MAX && cp == 0)) {
			return 0;
		} else {
			return ret;
		}
	} else {
		*cp = GRAPHEME_INVALID_CODEPOINT;
		return 0;
	}
}

inline size_t
set_codepoint(uint_least32_t cp, void *str, size_t len, size_t offset)
{
	if (str == NULL || len == 0) {
		return 1;
	}

	if (offset < len) {
		((uint_least32_t *)str)[offset] = cp;
		return 1;
	} else {
		return 0;
	}
}

inline size_t
set_codepoint_utf8(uint_least32_t cp, void *str, size_t len, size_t offset)
{
	if (str == NULL || len == 0) {
		return grapheme_encode_utf8(cp, NULL, 0);
	}

	if (offset < len) {
		return grapheme_encode_utf8(cp, (char *)str + offset,
		                            len - offset);
	} else {
		return grapheme_encode_utf8(cp, NULL, 0);
	}
}
