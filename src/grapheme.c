/* See LICENSE file for copyright and license details. */
#include <stddef.h>
#include <stdlib.h>

#include "../gen/grapheme.h"
#include "../grapheme.h"

enum {
	GRAPHEME_STATE_RI_ODD = 1 << 0, /* odd number of RI's before the seam */
	GRAPHEME_STATE_EMOJI  = 1 << 1, /* within emoji modifier or zwj sequence */
};

int
lg_grapheme_isbreak(uint32_t a, uint32_t b, int *state)
{
	struct heisenstate prop[2] = { 0 };
	int s;

	/* skip printable ASCII */
	if ((a >= 0x20 && a <= 0x7E) &&
	    (b >= 0x20 && b <= 0x7E)) {
		return 1;
	}

	/* set internal state based on given state-pointer */
	s = (state != NULL) ? *state : 0;

	/*
	 * Apply grapheme cluster breaking algorithm (UAX #29), see
	 * http://unicode.org/reports/tr29/#Grapheme_Cluster_Boundary_Rules
	 */

	/*
	 * update state
	 */
	if (has_property(b, &prop[1], grapheme_prop, GRAPHEME_PROP_REGIONAL_INDICATOR)) {
		if (has_property(a, &prop[0], grapheme_prop, GRAPHEME_PROP_REGIONAL_INDICATOR)) {
			/* one more RI is on the left side of the seam */
			s ^= GRAPHEME_STATE_RI_ODD;
		} else {
			/* an RI appeared on the right side but the left
			   side is not an RI, reset state (0 is even) */
			s &= ~GRAPHEME_STATE_RI_ODD;
		}
	}
	if (!(*state & GRAPHEME_STATE_EMOJI) &&
	    ((has_property(a, &prop[0], grapheme_prop, GRAPHEME_PROP_EXTENDED_PICTOGRAPHIC) &&
	      has_property(b, &prop[1], grapheme_prop, GRAPHEME_PROP_ZWJ)) ||
             (has_property(a, &prop[0], grapheme_prop, GRAPHEME_PROP_EXTENDED_PICTOGRAPHIC) &&
	      has_property(b, &prop[1], grapheme_prop, GRAPHEME_PROP_EXTEND)))) {
		s |= GRAPHEME_STATE_EMOJI;
	} else if ((*state & GRAPHEME_STATE_EMOJI) &&
	           ((has_property(a, &prop[0], grapheme_prop, GRAPHEME_PROP_ZWJ) &&
		     has_property(b, &prop[1], grapheme_prop, GRAPHEME_PROP_EXTENDED_PICTOGRAPHIC)) ||
	            (has_property(a, &prop[0], grapheme_prop, GRAPHEME_PROP_EXTEND) &&
		     has_property(b, &prop[1], grapheme_prop, GRAPHEME_PROP_EXTEND)) ||
	            (has_property(a, &prop[0], grapheme_prop, GRAPHEME_PROP_EXTEND) &&
		     has_property(b, &prop[1], grapheme_prop, GRAPHEME_PROP_ZWJ)) ||
	            (has_property(a, &prop[0], grapheme_prop, GRAPHEME_PROP_EXTENDED_PICTOGRAPHIC) &&
		     has_property(b, &prop[1], grapheme_prop, GRAPHEME_PROP_ZWJ)) ||
	            (has_property(a, &prop[0], grapheme_prop, GRAPHEME_PROP_EXTENDED_PICTOGRAPHIC) &&
		     has_property(b, &prop[1], grapheme_prop, GRAPHEME_PROP_EXTEND)))) {
		/* GRAPHEME_STATE_EMOJI remains */
	} else {
		s &= ~GRAPHEME_STATE_EMOJI;
	}

	/* write updated state to state-pointer, if given */
	if (state != NULL) {
		*state = s;
	}

	/*
	 * apply rules
	 */

	/* skip GB1 and GB2, as they are never satisfied here */

	/* GB3 */
	if (has_property(a, &prop[0], grapheme_prop, GRAPHEME_PROP_CR) &&
	    has_property(b, &prop[1], grapheme_prop, GRAPHEME_PROP_LF)) {
		return 0;
	}

	/* GB4 */
	if (has_property(a, &prop[0], grapheme_prop, GRAPHEME_PROP_CONTROL) ||
	    has_property(a, &prop[0], grapheme_prop, GRAPHEME_PROP_CR) ||
	    has_property(a, &prop[0], grapheme_prop, GRAPHEME_PROP_LF)) {
		return 1;
	}

	/* GB5 */
	if (has_property(b, &prop[1], grapheme_prop, GRAPHEME_PROP_CONTROL) ||
	    has_property(b, &prop[1], grapheme_prop, GRAPHEME_PROP_CR) ||
	    has_property(b, &prop[1], grapheme_prop, GRAPHEME_PROP_LF)) {
		return 1;
	}

	/* GB6 */
	if (has_property(a, &prop[0], grapheme_prop, GRAPHEME_PROP_HANGUL_L) &&
	    (has_property(b, &prop[1], grapheme_prop, GRAPHEME_PROP_HANGUL_L) ||
	     has_property(b, &prop[1], grapheme_prop, GRAPHEME_PROP_HANGUL_V) ||
	     has_property(b, &prop[1], grapheme_prop, GRAPHEME_PROP_HANGUL_LV) ||
	     has_property(b, &prop[1], grapheme_prop, GRAPHEME_PROP_HANGUL_LVT))) {
		return 0;
	}

	/* GB7 */
	if ((has_property(a, &prop[0], grapheme_prop, GRAPHEME_PROP_HANGUL_LV) ||
	     has_property(a, &prop[0], grapheme_prop, GRAPHEME_PROP_HANGUL_V)) &&
	    (has_property(b, &prop[1], grapheme_prop, GRAPHEME_PROP_HANGUL_V) ||
	     has_property(b, &prop[1], grapheme_prop, GRAPHEME_PROP_HANGUL_T))) {
		return 0;
	}

	/* GB8 */
	if ((has_property(a, &prop[0], grapheme_prop, GRAPHEME_PROP_HANGUL_LVT) ||
	     has_property(a, &prop[0], grapheme_prop, GRAPHEME_PROP_HANGUL_T)) &&
	    has_property(b, &prop[1], grapheme_prop, GRAPHEME_PROP_HANGUL_T)) {
		return 0;
	}

	/* GB9 */
	if (has_property(b, &prop[1], grapheme_prop, GRAPHEME_PROP_EXTEND) ||
	    has_property(b, &prop[1], grapheme_prop, GRAPHEME_PROP_ZWJ)) {
		return 0;
	}

	/* GB9a */
	if (has_property(b, &prop[1], grapheme_prop, GRAPHEME_PROP_SPACINGMARK)) {
		return 0;
	}

	/* GB9b */
	if (has_property(a, &prop[0], grapheme_prop, GRAPHEME_PROP_PREPEND)) {
		return 0;
	}

	/* GB11 */
	if ((s & GRAPHEME_STATE_EMOJI) &&
	    has_property(a, &prop[0], grapheme_prop, GRAPHEME_PROP_ZWJ) &&
	    has_property(b, &prop[1], grapheme_prop, GRAPHEME_PROP_EXTENDED_PICTOGRAPHIC)) {
		return 0;
	}

	/* GB12/GB13 */
	if (has_property(a, &prop[0], grapheme_prop, GRAPHEME_PROP_REGIONAL_INDICATOR) &&
	    has_property(b, &prop[1], grapheme_prop, GRAPHEME_PROP_REGIONAL_INDICATOR) &&
	    (s & GRAPHEME_STATE_RI_ODD)) {
		return 0;
	}

	/* GB999 */
	return 1;
}

size_t
lg_grapheme_nextbreak(const char *str)
{
	uint32_t cp0, cp1;
	size_t ret, len = 0;
	int state = 0;

	if (str == NULL) {
		return 0;
	}

	/*
	 * lg_utf8_decode, when it encounters an unexpected byte,
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
	len += lg_utf8_decode((uint8_t *)str, 5, &cp0);
	if (cp0 == LG_CODEPOINT_INVALID) {
		return len;
	}

	while (cp0 != 0) {
		/* get next code point */
		ret = lg_utf8_decode((uint8_t *)(str + len), 5, &cp1);

		if (cp1 == LG_CODEPOINT_INVALID ||
		    lg_grapheme_isbreak(cp0, cp1, &state)) {
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
