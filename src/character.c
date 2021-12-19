/* See LICENSE file for copyright and license details. */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "../gen/character-prop.h"
#include "../grapheme.h"
#include "util.h"

enum {
	CHARACTER_FLAG_RI_ODD = 1 << 0, /* odd number of RI's before the seam */
	CHARACTER_FLAG_EMOJI  = 1 << 1, /* within emoji modifier or zwj sequence */
};

bool
grapheme_is_character_break(uint_least32_t cp0, uint_least32_t cp1, GRAPHEME_STATE *state)
{
	struct grapheme_internal_heisenstate *p[2] = { 0 };
	uint_least16_t flags = 0;
	bool isbreak = true;

	/* set state depending on state pointer */
	if (state != NULL) {
		p[0] = &(state->cp0);
		p[1] = &(state->cp1);
		flags = state->flags;
	}

	/* skip printable ASCII */
	if ((cp0 >= 0x20 && cp0 <= 0x7E) &&
	    (cp1 >= 0x20 && cp1 <= 0x7E)) {
		goto hasbreak;
	}

	/*
	 * Apply grapheme cluster breaking algorithm (UAX #29), see
	 * http://unicode.org/reports/tr29/#Grapheme_Cluster_Boundary_Rules
	 */

	/*
	 * update flags, if state-pointer given
	 */
	if (has_property(cp1, p[1], character_prop, CHARACTER_PROP_REGIONAL_INDICATOR)) {
		if (has_property(cp0, p[0], character_prop, CHARACTER_PROP_REGIONAL_INDICATOR)) {
			/* one more RI is on the left side of the seam, flip state */
			flags ^= CHARACTER_FLAG_RI_ODD;
		} else {
			/* an RI appeared on the right side but the left
			   side is not an RI, reset state (number 0 is even) */
			flags &= ~CHARACTER_FLAG_RI_ODD;
		}
	}
	if (!(flags & CHARACTER_FLAG_EMOJI) &&
	    ((has_property(cp0, p[0], character_prop, CHARACTER_PROP_EXTENDED_PICTOGRAPHIC) &&
	      has_property(cp1, p[1], character_prop, CHARACTER_PROP_ZWJ)) ||
             (has_property(cp0, p[0], character_prop, CHARACTER_PROP_EXTENDED_PICTOGRAPHIC) &&
	      has_property(cp1, p[1], character_prop, CHARACTER_PROP_EXTEND)))) {
		flags |= CHARACTER_FLAG_EMOJI;
	} else if ((flags & CHARACTER_FLAG_EMOJI) &&
	           ((has_property(cp0, p[0], character_prop, CHARACTER_PROP_ZWJ) &&
		     has_property(cp1, p[1], character_prop, CHARACTER_PROP_EXTENDED_PICTOGRAPHIC)) ||
	            (has_property(cp0, p[0], character_prop, CHARACTER_PROP_EXTEND) &&
		     has_property(cp1, p[1], character_prop, CHARACTER_PROP_EXTEND)) ||
	            (has_property(cp0, p[0], character_prop, CHARACTER_PROP_EXTEND) &&
		     has_property(cp1, p[1], character_prop, CHARACTER_PROP_ZWJ)) ||
	            (has_property(cp0, p[0], character_prop, CHARACTER_PROP_EXTENDED_PICTOGRAPHIC) &&
		     has_property(cp1, p[1], character_prop, CHARACTER_PROP_ZWJ)) ||
	            (has_property(cp0, p[0], character_prop, CHARACTER_PROP_EXTENDED_PICTOGRAPHIC) &&
		     has_property(cp1, p[1], character_prop, CHARACTER_PROP_EXTEND)))) {
		/* CHARACTER_FLAG_EMOJI remains */
	} else {
		flags &= ~CHARACTER_FLAG_EMOJI;
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
	if (has_property(cp0, p[0], character_prop, CHARACTER_PROP_CR) &&
	    has_property(cp1, p[1], character_prop, CHARACTER_PROP_LF)) {
		goto nobreak;
	}

	/* GB4 */
	if (has_property(cp0, p[0], character_prop, CHARACTER_PROP_CONTROL) ||
	    has_property(cp0, p[0], character_prop, CHARACTER_PROP_CR) ||
	    has_property(cp0, p[0], character_prop, CHARACTER_PROP_LF)) {
		goto hasbreak;
	}

	/* GB5 */
	if (has_property(cp1, p[1], character_prop, CHARACTER_PROP_CONTROL) ||
	    has_property(cp1, p[1], character_prop, CHARACTER_PROP_CR) ||
	    has_property(cp1, p[1], character_prop, CHARACTER_PROP_LF)) {
		goto hasbreak;
	}

	/* GB6 */
	if (has_property(cp0, p[0], character_prop, CHARACTER_PROP_HANGUL_L) &&
	    (has_property(cp1, p[1], character_prop, CHARACTER_PROP_HANGUL_L) ||
	     has_property(cp1, p[1], character_prop, CHARACTER_PROP_HANGUL_V) ||
	     has_property(cp1, p[1], character_prop, CHARACTER_PROP_HANGUL_LV) ||

	     has_property(cp1, p[1], character_prop, CHARACTER_PROP_HANGUL_LVT))) {
		goto nobreak;
	}

	/* GB7 */
	if ((has_property(cp0, p[0], character_prop, CHARACTER_PROP_HANGUL_LV) ||
	     has_property(cp0, p[0], character_prop, CHARACTER_PROP_HANGUL_V)) &&
	    (has_property(cp1, p[1], character_prop, CHARACTER_PROP_HANGUL_V) ||
	     has_property(cp1, p[1], character_prop, CHARACTER_PROP_HANGUL_T))) {
		goto nobreak;
	}

	/* GB8 */
	if ((has_property(cp0, p[0], character_prop, CHARACTER_PROP_HANGUL_LVT) ||
	     has_property(cp0, p[0], character_prop, CHARACTER_PROP_HANGUL_T)) &&
	    has_property(cp1, p[1], character_prop, CHARACTER_PROP_HANGUL_T)) {
		goto nobreak;
	}

	/* GB9 */
	if (has_property(cp1, p[1], character_prop, CHARACTER_PROP_EXTEND) ||
	    has_property(cp1, p[1], character_prop, CHARACTER_PROP_ZWJ)) {
		goto nobreak;
	}

	/* GB9a */
	if (has_property(cp1, p[1], character_prop, CHARACTER_PROP_SPACINGMARK)) {
		goto nobreak;
	}

	/* GB9b */
	if (has_property(cp0, p[0], character_prop, CHARACTER_PROP_PREPEND)) {
		goto nobreak;
	}

	/* GB11 */
	if ((flags & CHARACTER_FLAG_EMOJI) &&
	    has_property(cp0, p[0], character_prop, CHARACTER_PROP_ZWJ) &&
	    has_property(cp1, p[1], character_prop, CHARACTER_PROP_EXTENDED_PICTOGRAPHIC)) {
		goto nobreak;
	}

	/* GB12/GB13 */
	if (has_property(cp0, p[0], character_prop, CHARACTER_PROP_REGIONAL_INDICATOR) &&
	    has_property(cp1, p[1], character_prop, CHARACTER_PROP_REGIONAL_INDICATOR) &&
	    (flags & CHARACTER_FLAG_RI_ODD)) {
		goto nobreak;
	}

	/* GB999 */
	goto hasbreak;
nobreak:
	isbreak = false;
hasbreak:
	if (state != NULL) {
		/* move b-state to a-state, discard b-state */
		memcpy(&(state->cp0), &(state->cp1), sizeof(state->cp0));
		memset(&(state->cp1), 0, sizeof(state->cp1));

		/* reset flags */
		if (isbreak) {
			state->flags = 0;
		}
	}

	return isbreak;
}

size_t
grapheme_next_character_break(const char *str, size_t len)
{
	GRAPHEME_STATE state = { 0 };
	uint_least32_t cp0 = 0, cp1 = 0;
	size_t off, ret;

	if (str == NULL || len == 0) {
		return 0;
	}

	for (off = 0; (len == SIZE_MAX) || off < len; off += ret) {
		cp0 = cp1;
		ret = grapheme_decode_utf8(str + off, (len == SIZE_MAX) ?
		                           SIZE_MAX : len - off, &cp1);

		if (len != SIZE_MAX && ret > (len - off)) {
			/* string ended abruptly, simply accept cropping */
			ret = len - off;
		}

		if (len == SIZE_MAX && cp1 == 0) {
			/* we hit a NUL-byte and are done */
			break;
		}

		if (off == 0) {
			/*
			 * we skip the first round, as we need both
			 * cp0 and cp1 to be initialized
			 */
			continue;
		} else if (grapheme_is_character_break(cp0, cp1, &state)) {
			break;
		}
	}

	return off;
}
