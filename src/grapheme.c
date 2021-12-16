/* See LICENSE file for copyright and license details. */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "../gen/grapheme.h"
#include "../grapheme.h"
#include "util.h"

enum {
	GRAPHEME_FLAG_RI_ODD = 1 << 0, /* odd number of RI's before the seam */
	GRAPHEME_FLAG_EMOJI  = 1 << 1, /* within emoji modifier or zwj sequence */
};

bool
lg_grapheme_isbreak(uint_least32_t a, uint_least32_t b, LG_SEGMENTATION_STATE *state)
{
	struct lg_internal_heisenstate *p[2] = { 0 };
	uint_least16_t flags = 0;
	bool isbreak = true;

	/* set state depending on state pointer */
	if (state != NULL) {
		p[0] = &(state->a);
		p[1] = &(state->b);
		flags = state->flags;
	}

	/* skip printable ASCII */
	if ((a >= 0x20 && a <= 0x7E) &&
	    (b >= 0x20 && b <= 0x7E)) {
		goto hasbreak;
	}

	/*
	 * Apply grapheme cluster breaking algorithm (UAX #29), see
	 * http://unicode.org/reports/tr29/#Grapheme_Cluster_Boundary_Rules
	 */

	/*
	 * update flags, if state-pointer given
	 */
	if (has_property(b, p[1], grapheme_prop, GRAPHEME_PROP_REGIONAL_INDICATOR)) {
		if (has_property(a, p[0], grapheme_prop, GRAPHEME_PROP_REGIONAL_INDICATOR)) {
			/* one more RI is on the left side of the seam, flip state */
			flags ^= GRAPHEME_FLAG_RI_ODD;
		} else {
			/* an RI appeared on the right side but the left
			   side is not an RI, reset state (number 0 is even) */
			flags &= ~GRAPHEME_FLAG_RI_ODD;
		}
	}
	if (!(flags & GRAPHEME_FLAG_EMOJI) &&
	    ((has_property(a, p[0], grapheme_prop, GRAPHEME_PROP_EXTENDED_PICTOGRAPHIC) &&
	      has_property(b, p[1], grapheme_prop, GRAPHEME_PROP_ZWJ)) ||
             (has_property(a, p[0], grapheme_prop, GRAPHEME_PROP_EXTENDED_PICTOGRAPHIC) &&
	      has_property(b, p[1], grapheme_prop, GRAPHEME_PROP_EXTEND)))) {
		flags |= GRAPHEME_FLAG_EMOJI;
	} else if ((flags & GRAPHEME_FLAG_EMOJI) &&
	           ((has_property(a, p[0], grapheme_prop, GRAPHEME_PROP_ZWJ) &&
		     has_property(b, p[1], grapheme_prop, GRAPHEME_PROP_EXTENDED_PICTOGRAPHIC)) ||
	            (has_property(a, p[0], grapheme_prop, GRAPHEME_PROP_EXTEND) &&
		     has_property(b, p[1], grapheme_prop, GRAPHEME_PROP_EXTEND)) ||
	            (has_property(a, p[0], grapheme_prop, GRAPHEME_PROP_EXTEND) &&
		     has_property(b, p[1], grapheme_prop, GRAPHEME_PROP_ZWJ)) ||
	            (has_property(a, p[0], grapheme_prop, GRAPHEME_PROP_EXTENDED_PICTOGRAPHIC) &&
		     has_property(b, p[1], grapheme_prop, GRAPHEME_PROP_ZWJ)) ||
	            (has_property(a, p[0], grapheme_prop, GRAPHEME_PROP_EXTENDED_PICTOGRAPHIC) &&
		     has_property(b, p[1], grapheme_prop, GRAPHEME_PROP_EXTEND)))) {
		/* GRAPHEME_FLAG_EMOJI remains */
	} else {
		flags &= ~GRAPHEME_FLAG_EMOJI;
	}

	/* write updated flags to state, if given */
	if (state != NULL) {
		state->flags = flags;
	}

	/*
	 * apply rules
	 */

	/* skip GB1 and GB2, as they are never satisfied here */

	/* GB3 */
	if (has_property(a, p[0], grapheme_prop, GRAPHEME_PROP_CR) &&
	    has_property(b, p[1], grapheme_prop, GRAPHEME_PROP_LF)) {
		goto nobreak;
	}

	/* GB4 */
	if (has_property(a, p[0], grapheme_prop, GRAPHEME_PROP_CONTROL) ||
	    has_property(a, p[0], grapheme_prop, GRAPHEME_PROP_CR) ||
	    has_property(a, p[0], grapheme_prop, GRAPHEME_PROP_LF)) {
		goto hasbreak;
	}

	/* GB5 */
	if (has_property(b, p[1], grapheme_prop, GRAPHEME_PROP_CONTROL) ||
	    has_property(b, p[1], grapheme_prop, GRAPHEME_PROP_CR) ||
	    has_property(b, p[1], grapheme_prop, GRAPHEME_PROP_LF)) {
		goto hasbreak;
	}

	/* GB6 */
	if (has_property(a, p[0], grapheme_prop, GRAPHEME_PROP_HANGUL_L) &&
	    (has_property(b, p[1], grapheme_prop, GRAPHEME_PROP_HANGUL_L) ||
	     has_property(b, p[1], grapheme_prop, GRAPHEME_PROP_HANGUL_V) ||
	     has_property(b, p[1], grapheme_prop, GRAPHEME_PROP_HANGUL_LV) ||

	     has_property(b, p[1], grapheme_prop, GRAPHEME_PROP_HANGUL_LVT))) {
		goto nobreak;
	}

	/* GB7 */
	if ((has_property(a, p[0], grapheme_prop, GRAPHEME_PROP_HANGUL_LV) ||
	     has_property(a, p[0], grapheme_prop, GRAPHEME_PROP_HANGUL_V)) &&
	    (has_property(b, p[1], grapheme_prop, GRAPHEME_PROP_HANGUL_V) ||
	     has_property(b, p[1], grapheme_prop, GRAPHEME_PROP_HANGUL_T))) {
		goto nobreak;
	}

	/* GB8 */
	if ((has_property(a, p[0], grapheme_prop, GRAPHEME_PROP_HANGUL_LVT) ||
	     has_property(a, p[0], grapheme_prop, GRAPHEME_PROP_HANGUL_T)) &&
	    has_property(b, p[1], grapheme_prop, GRAPHEME_PROP_HANGUL_T)) {
		goto nobreak;
	}

	/* GB9 */
	if (has_property(b, p[1], grapheme_prop, GRAPHEME_PROP_EXTEND) ||
	    has_property(b, p[1], grapheme_prop, GRAPHEME_PROP_ZWJ)) {
		goto nobreak;
	}

	/* GB9a */
	if (has_property(b, p[1], grapheme_prop, GRAPHEME_PROP_SPACINGMARK)) {
		goto nobreak;
	}

	/* GB9b */
	if (has_property(a, p[0], grapheme_prop, GRAPHEME_PROP_PREPEND)) {
		goto nobreak;
	}

	/* GB11 */
	if ((flags & GRAPHEME_FLAG_EMOJI) &&
	    has_property(a, p[0], grapheme_prop, GRAPHEME_PROP_ZWJ) &&
	    has_property(b, p[1], grapheme_prop, GRAPHEME_PROP_EXTENDED_PICTOGRAPHIC)) {
		goto nobreak;
	}

	/* GB12/GB13 */
	if (has_property(a, p[0], grapheme_prop, GRAPHEME_PROP_REGIONAL_INDICATOR) &&
	    has_property(b, p[1], grapheme_prop, GRAPHEME_PROP_REGIONAL_INDICATOR) &&
	    (flags & GRAPHEME_FLAG_RI_ODD)) {
		goto nobreak;
	}

	/* GB999 */
	goto hasbreak;
nobreak:
	isbreak = false;
hasbreak:
	if (state != NULL) {
		/* move b-state to a-state, discard b-state */
		memcpy(&(state->a), &(state->b), sizeof(state->a));
		memset(&(state->b), 0, sizeof(state->b));

		/* reset flags */
		if (isbreak) {
			state->flags = 0;
		}
	}

	return isbreak;
}

size_t
lg_grapheme_nextbreak(const char *str)
{
	uint_least32_t cp0, cp1;
	size_t ret, len = 0;
	LG_SEGMENTATION_STATE state = { 0 };

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
	len += lg_utf8_decode(str, 5, &cp0);
	if (cp0 == LG_CODEPOINT_INVALID) {
		return len;
	}

	while (cp0 != 0) {
		/* get next code point */
		ret = lg_utf8_decode(str + len, 5, &cp1);

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
