/* See LICENSE file for copyright and license details. */
#include <stddef.h>
#include <stdlib.h>

#include "codepoint.h"
#include "boundary.h"

size_t
grapheme_len(const char *str)
{
	Codepoint cp0, cp1;
	size_t ret, len = 0;
	int state = 0;

	/* get first code point */
	if ((ret = cp_decode((const uint8_t *)str, &cp0)) == 0) {
		return len;
	}
	len += ret;

	while (cp0 != 0) {
		/* get next codepoint */
		if ((ret = cp_decode((const uint8_t *)(str + len), &cp1)) == 0) {
			break;
		}

		if (boundary(cp0, cp1, &state)) {
			/* we have a breakpoint */
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
