/* See LICENSE file for copyright and license details. */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "../gen/properties.h"
#include "../grapheme.h"
#include "util.h"

static const uint_least16_t dont_break[NUM_BREAK_PROPS] = {
	[BREAK_PROP_OTHER] =
		UINT16_C(1 << BREAK_PROP_EXTEND)       | /* GB9  */
		UINT16_C(1 << BREAK_PROP_ZWJ)          | /* GB9  */
		UINT16_C(1 << BREAK_PROP_SPACINGMARK),   /* GB9a */
	[BREAK_PROP_CR] =
		UINT16_C(1 << BREAK_PROP_LF),            /* GB3  */
	[BREAK_PROP_EXTEND] =
		UINT16_C(1 << BREAK_PROP_EXTEND)       | /* GB9  */
		UINT16_C(1 << BREAK_PROP_ZWJ)          | /* GB9  */
		UINT16_C(1 << BREAK_PROP_SPACINGMARK),   /* GB9a */
	[BREAK_PROP_EXTENDED_PICTOGRAPHIC] =
		UINT16_C(1 << BREAK_PROP_EXTEND)       | /* GB9  */
		UINT16_C(1 << BREAK_PROP_ZWJ)          | /* GB9  */
		UINT16_C(1 << BREAK_PROP_SPACINGMARK),   /* GB9a */
	[BREAK_PROP_HANGUL_L] =
		UINT16_C(1 << BREAK_PROP_HANGUL_L)     | /* GB6  */
		UINT16_C(1 << BREAK_PROP_HANGUL_V)     | /* GB6  */
		UINT16_C(1 << BREAK_PROP_HANGUL_LV)    | /* GB6  */
		UINT16_C(1 << BREAK_PROP_HANGUL_LVT)   | /* GB6  */
		UINT16_C(1 << BREAK_PROP_EXTEND)       | /* GB9  */
		UINT16_C(1 << BREAK_PROP_ZWJ)          | /* GB9  */
		UINT16_C(1 << BREAK_PROP_SPACINGMARK),   /* GB9a */
	[BREAK_PROP_HANGUL_V] =
		UINT16_C(1 << BREAK_PROP_HANGUL_V)     | /* GB7  */
		UINT16_C(1 << BREAK_PROP_HANGUL_T)     | /* GB7  */
		UINT16_C(1 << BREAK_PROP_EXTEND)       | /* GB9  */
		UINT16_C(1 << BREAK_PROP_ZWJ)          | /* GB9  */
		UINT16_C(1 << BREAK_PROP_SPACINGMARK),   /* GB9a */
	[BREAK_PROP_HANGUL_T] =
		UINT16_C(1 << BREAK_PROP_HANGUL_T)     | /* GB8  */
		UINT16_C(1 << BREAK_PROP_EXTEND)       | /* GB9  */
		UINT16_C(1 << BREAK_PROP_ZWJ)          | /* GB9  */
		UINT16_C(1 << BREAK_PROP_SPACINGMARK),   /* GB9a */
	[BREAK_PROP_HANGUL_LV] =
		UINT16_C(1 << BREAK_PROP_HANGUL_V)     | /* GB7  */
		UINT16_C(1 << BREAK_PROP_HANGUL_T)     | /* GB7  */
		UINT16_C(1 << BREAK_PROP_EXTEND)       | /* GB9  */
		UINT16_C(1 << BREAK_PROP_ZWJ)          | /* GB9  */
		UINT16_C(1 << BREAK_PROP_SPACINGMARK),   /* GB9a */
	[BREAK_PROP_HANGUL_LVT] =
		UINT16_C(1 << BREAK_PROP_HANGUL_T)     | /* GB8  */
		UINT16_C(1 << BREAK_PROP_EXTEND)       | /* GB9  */
		UINT16_C(1 << BREAK_PROP_ZWJ)          | /* GB9  */
		UINT16_C(1 << BREAK_PROP_SPACINGMARK),   /* GB9a */
	[BREAK_PROP_PREPEND] =
		UINT16_C(1 << BREAK_PROP_EXTEND)       | /* GB9  */
		UINT16_C(1 << BREAK_PROP_ZWJ)          | /* GB9  */
		UINT16_C(1 << BREAK_PROP_SPACINGMARK)  | /* GB9a */
		(UINT16_C(0xFFFF) &
		 ~(UINT16_C(1 << BREAK_PROP_CR)      |
		   UINT16_C(1 << BREAK_PROP_LF)      |
		   UINT16_C(1 << BREAK_PROP_CONTROL)
		  )
		),                                           /* GB9b */
	[BREAK_PROP_REGIONAL_INDICATOR] =
		UINT16_C(1 << BREAK_PROP_EXTEND)       | /* GB9  */
		UINT16_C(1 << BREAK_PROP_ZWJ)          | /* GB9  */
		UINT16_C(1 << BREAK_PROP_SPACINGMARK),   /* GB9a */
	[BREAK_PROP_SPACINGMARK] =
		UINT16_C(1 << BREAK_PROP_EXTEND)       | /* GB9  */
		UINT16_C(1 << BREAK_PROP_ZWJ)          | /* GB9  */
		UINT16_C(1 << BREAK_PROP_SPACINGMARK),   /* GB9a */
	[BREAK_PROP_ZWJ] =
		UINT16_C(1 << BREAK_PROP_EXTEND)       | /* GB9  */
		UINT16_C(1 << BREAK_PROP_ZWJ)          | /* GB9  */
		UINT16_C(1 << BREAK_PROP_SPACINGMARK),   /* GB9a */
};
static const uint_least16_t flag_update_gb11[2 * NUM_BREAK_PROPS] = {
	[BREAK_PROP_EXTENDED_PICTOGRAPHIC] =
		UINT16_C(1 << BREAK_PROP_ZWJ)                   |
		UINT16_C(1 << BREAK_PROP_EXTEND),
	[BREAK_PROP_ZWJ + NUM_BREAK_PROPS] =
		UINT16_C(1 << BREAK_PROP_EXTENDED_PICTOGRAPHIC),
	[BREAK_PROP_EXTEND + NUM_BREAK_PROPS] =
		UINT16_C(1 << BREAK_PROP_EXTEND)                |
		UINT16_C(1 << BREAK_PROP_ZWJ),
	[BREAK_PROP_EXTENDED_PICTOGRAPHIC + NUM_BREAK_PROPS] =
		UINT16_C(1 << BREAK_PROP_ZWJ)                   |
		UINT16_C(1 << BREAK_PROP_EXTEND),
};
static const uint_least16_t dont_break_gb11[2 * NUM_BREAK_PROPS] = {
	[BREAK_PROP_ZWJ + NUM_BREAK_PROPS] =
		UINT16_C(1 << BREAK_PROP_EXTENDED_PICTOGRAPHIC),
};
static const uint_least16_t flag_update_gb12_13[2 * NUM_BREAK_PROPS] = {
	[BREAK_PROP_REGIONAL_INDICATOR] =
		UINT16_C(1 << BREAK_PROP_REGIONAL_INDICATOR),
};
static const uint_least16_t dont_break_gb12_13[2 * NUM_BREAK_PROPS] = {
	[BREAK_PROP_REGIONAL_INDICATOR + NUM_BREAK_PROPS] =
		UINT16_C(1 << BREAK_PROP_REGIONAL_INDICATOR),
};

static enum break_property
get_break_prop(uint_least32_t cp)
{
	if (likely(cp <= 0x10FFFF)) {
		return prop[minor[major[cp >> 8] + (cp & 0xff)]].break_property;
	} else {
		return BREAK_PROP_OTHER;
	}
}

bool
grapheme_is_character_break(uint_least32_t cp0, uint_least32_t cp1, GRAPHEME_STATE *state)
{
	enum break_property cp0_prop, cp1_prop;
	bool notbreak = false;

	if (likely(state)) {
		if (likely(state->prop_set)) {
			cp0_prop = state->prop;
		} else {
			cp0_prop = get_break_prop(cp0);
		}
		cp1_prop = get_break_prop(cp1);

		/* preserve prop of right codepoint for next iteration */
		state->prop = cp1_prop;
		state->prop_set = true;

		/* update flags */
		state->gb11_flag = flag_update_gb11[cp0_prop +
		                                    NUM_BREAK_PROPS *
		                                    state->gb11_flag] &
	                           UINT16_C(1 << cp1_prop);
		state->gb12_13_flag = flag_update_gb12_13[cp0_prop +
		                                          NUM_BREAK_PROPS *
		                                          state->gb12_13_flag] &
		                      UINT16_C(1 << cp1_prop);

		/*
		 * Apply grapheme cluster breaking algorithm (UAX #29), see
		 * http://unicode.org/reports/tr29/#Grapheme_Cluster_Boundary_Rules
		 */
		notbreak = (dont_break[cp0_prop] & UINT16_C(1 << cp1_prop)) ||
		           (dont_break_gb11[cp0_prop + state->gb11_flag *
		                            NUM_BREAK_PROPS] &
		            UINT16_C(1 << cp1_prop)) ||
		           (dont_break_gb12_13[cp0_prop + state->gb12_13_flag *
		                               NUM_BREAK_PROPS] &
		            UINT16_C(1 << cp1_prop));

		/* update or reset flags (when we have a break) */
		if (likely(!notbreak)) {
			state->gb11_flag = state->gb12_13_flag = false;
		}
	} else {
		cp0_prop = get_break_prop(cp0);
		cp1_prop = get_break_prop(cp1);

		/*
		 * Apply grapheme cluster breaking algorithm (UAX #29), see
		 * http://unicode.org/reports/tr29/#Grapheme_Cluster_Boundary_Rules
		 *
		 * Given we have no state, this behaves as if the state-booleans
		 * were all set to false
		 */
		notbreak = (dont_break[cp0_prop] & UINT16_C(1 << cp1_prop)) ||
		           (dont_break_gb11[cp0_prop] & UINT16_C(1 << cp1_prop)) ||
		           (dont_break_gb12_13[cp0_prop] & UINT16_C(1 << cp1_prop));
	}

	return !notbreak;
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
