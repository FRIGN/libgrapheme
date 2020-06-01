/* See LICENSE file for copyright and license details. */
#include <stddef.h>
#include <stdlib.h>

#include "../grapheme.h"

size_t
grapheme_len(const char *str)
{
	uint32_t cp0, cp1;
	size_t ret, len = 0;
	int state = 0;

	if (str == NULL) {
		return 0;
	}

	/*
	 * grapheme_cp_decode, when it encounters an unexpected byte,
	 * does not count it to the error and instead assumes that the
	 * unexpected byte is the beginning of a new sequence.
	 * This way, when the string ends with a null byte, we never
	 * miss it, even if the previous UTF-8 sequence terminates
	 * unexpectedly, as it would either act as an unexpected byte,
	 * saved for later, or as a null byte itself, that we can catch.
	 * We pass 5 to the length, as we will never read beyond
	 * the null byte for the reasons given above.
	 */

	/* get first code point */
	len += grapheme_cp_decode(&cp0, (uint8_t *)str, 5);
	if (cp0 == GRAPHEME_CP_INVALID) {
		return len;
	}

	while (cp0 != 0) {
		/* get next code point */
		ret = grapheme_cp_decode(&cp1, (uint8_t *)(str + len), 5);

		if (cp1 == GRAPHEME_CP_INVALID ||
		    grapheme_boundary(cp0, cp1, &state)) {
			/* we read an invalid cp or have a breakpoint */
			break;
		} else {
			/* we don't have a breakpoint, continue */
			len += ret;
		}

		/* prepare next round */
		cp0 = cp1;
	}

	return len;
}
