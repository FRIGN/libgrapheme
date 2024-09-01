#include <stdio.h>

/* See LICENSE file for copyright and license details. */
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>

#include "../gen/character.h"
#include "../grapheme.h"
#include "util.h"

struct character_break_state {
	uint_least8_t prop;
	bool prop_set;
	bool gb11_flag;
	bool gb12_13_flag;
	uint_least8_t gb9c_level;
};

static const uint_least32_t dont_break[NUM_CHAR_BREAK_PROPS] = {
	[CHAR_BREAK_PROP_OTHER] =
		UINT32_C(1) << CHAR_BREAK_PROP_EXTEND | /* GB9  */
		UINT32_C(1)
			<< CHAR_BREAK_PROP_BOTH_EXTEND_ICB_EXTEND | /* GB9  */
		UINT32_C(1)
			<< CHAR_BREAK_PROP_BOTH_EXTEND_ICB_LINKER |  /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_ZWJ |                 /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_BOTH_ZWJ_ICB_EXTEND | /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_SPACINGMARK,          /* GB9a */
	[CHAR_BREAK_PROP_ICB_CONSONANT] =
		UINT32_C(1) << CHAR_BREAK_PROP_EXTEND | /* GB9  */
		UINT32_C(1)
			<< CHAR_BREAK_PROP_BOTH_EXTEND_ICB_EXTEND | /* GB9  */
		UINT32_C(1)
			<< CHAR_BREAK_PROP_BOTH_EXTEND_ICB_LINKER |  /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_ZWJ |                 /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_BOTH_ZWJ_ICB_EXTEND | /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_SPACINGMARK,          /* GB9a */
	[CHAR_BREAK_PROP_ICB_EXTEND] =
		UINT32_C(1) << CHAR_BREAK_PROP_EXTEND | /* GB9  */
		UINT32_C(1)
			<< CHAR_BREAK_PROP_BOTH_EXTEND_ICB_EXTEND | /* GB9  */
		UINT32_C(1)
			<< CHAR_BREAK_PROP_BOTH_EXTEND_ICB_LINKER |  /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_ZWJ |                 /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_BOTH_ZWJ_ICB_EXTEND | /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_SPACINGMARK,          /* GB9a */
	[CHAR_BREAK_PROP_ICB_LINKER] =
		UINT32_C(1) << CHAR_BREAK_PROP_EXTEND | /* GB9  */
		UINT32_C(1)
			<< CHAR_BREAK_PROP_BOTH_EXTEND_ICB_EXTEND | /* GB9  */
		UINT32_C(1)
			<< CHAR_BREAK_PROP_BOTH_EXTEND_ICB_LINKER |  /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_ZWJ |                 /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_BOTH_ZWJ_ICB_EXTEND | /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_SPACINGMARK,          /* GB9a */
	[CHAR_BREAK_PROP_CR] = UINT32_C(1) << CHAR_BREAK_PROP_LF,    /* GB3  */
	[CHAR_BREAK_PROP_EXTEND] =
		UINT32_C(1) << CHAR_BREAK_PROP_EXTEND | /* GB9  */
		UINT32_C(1)
			<< CHAR_BREAK_PROP_BOTH_EXTEND_ICB_EXTEND | /* GB9  */
		UINT32_C(1)
			<< CHAR_BREAK_PROP_BOTH_EXTEND_ICB_LINKER |  /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_ZWJ |                 /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_BOTH_ZWJ_ICB_EXTEND | /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_SPACINGMARK,          /* GB9a */
	[CHAR_BREAK_PROP_BOTH_EXTEND_ICB_EXTEND] =
		UINT32_C(1) << CHAR_BREAK_PROP_EXTEND | /* GB9  */
		UINT32_C(1)
			<< CHAR_BREAK_PROP_BOTH_EXTEND_ICB_EXTEND | /* GB9  */
		UINT32_C(1)
			<< CHAR_BREAK_PROP_BOTH_EXTEND_ICB_LINKER |  /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_ZWJ |                 /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_BOTH_ZWJ_ICB_EXTEND | /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_SPACINGMARK,          /* GB9a */
	[CHAR_BREAK_PROP_BOTH_EXTEND_ICB_LINKER] =
		UINT32_C(1) << CHAR_BREAK_PROP_EXTEND | /* GB9  */
		UINT32_C(1)
			<< CHAR_BREAK_PROP_BOTH_EXTEND_ICB_EXTEND | /* GB9  */
		UINT32_C(1)
			<< CHAR_BREAK_PROP_BOTH_EXTEND_ICB_LINKER |  /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_ZWJ |                 /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_BOTH_ZWJ_ICB_EXTEND | /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_SPACINGMARK,          /* GB9a */
	[CHAR_BREAK_PROP_EXTENDED_PICTOGRAPHIC] =
		UINT32_C(1) << CHAR_BREAK_PROP_EXTEND | /* GB9  */
		UINT32_C(1)
			<< CHAR_BREAK_PROP_BOTH_EXTEND_ICB_EXTEND | /* GB9  */
		UINT32_C(1)
			<< CHAR_BREAK_PROP_BOTH_EXTEND_ICB_LINKER |  /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_ZWJ |                 /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_BOTH_ZWJ_ICB_EXTEND | /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_SPACINGMARK,          /* GB9a */
	[CHAR_BREAK_PROP_HANGUL_L] =
		UINT32_C(1) << CHAR_BREAK_PROP_HANGUL_L |   /* GB6  */
		UINT32_C(1) << CHAR_BREAK_PROP_HANGUL_V |   /* GB6  */
		UINT32_C(1) << CHAR_BREAK_PROP_HANGUL_LV |  /* GB6  */
		UINT32_C(1) << CHAR_BREAK_PROP_HANGUL_LVT | /* GB6  */
		UINT32_C(1) << CHAR_BREAK_PROP_EXTEND |     /* GB9  */
		UINT32_C(1)
			<< CHAR_BREAK_PROP_BOTH_EXTEND_ICB_EXTEND | /* GB9  */
		UINT32_C(1)
			<< CHAR_BREAK_PROP_BOTH_EXTEND_ICB_LINKER |  /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_ZWJ |                 /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_BOTH_ZWJ_ICB_EXTEND | /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_SPACINGMARK,          /* GB9a */
	[CHAR_BREAK_PROP_HANGUL_V] =
		UINT32_C(1) << CHAR_BREAK_PROP_HANGUL_V | /* GB7  */
		UINT32_C(1) << CHAR_BREAK_PROP_HANGUL_T | /* GB7  */
		UINT32_C(1) << CHAR_BREAK_PROP_EXTEND |   /* GB9  */
		UINT32_C(1)
			<< CHAR_BREAK_PROP_BOTH_EXTEND_ICB_EXTEND | /* GB9  */
		UINT32_C(1)
			<< CHAR_BREAK_PROP_BOTH_EXTEND_ICB_LINKER |  /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_ZWJ |                 /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_BOTH_ZWJ_ICB_EXTEND | /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_SPACINGMARK,          /* GB9a */
	[CHAR_BREAK_PROP_HANGUL_T] =
		UINT32_C(1) << CHAR_BREAK_PROP_HANGUL_T | /* GB8  */
		UINT32_C(1) << CHAR_BREAK_PROP_EXTEND |   /* GB9  */
		UINT32_C(1)
			<< CHAR_BREAK_PROP_BOTH_EXTEND_ICB_EXTEND | /* GB9  */
		UINT32_C(1)
			<< CHAR_BREAK_PROP_BOTH_EXTEND_ICB_LINKER |  /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_ZWJ |                 /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_BOTH_ZWJ_ICB_EXTEND | /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_SPACINGMARK,          /* GB9a */
	[CHAR_BREAK_PROP_HANGUL_LV] =
		UINT32_C(1) << CHAR_BREAK_PROP_HANGUL_V | /* GB7  */
		UINT32_C(1) << CHAR_BREAK_PROP_HANGUL_T | /* GB7  */
		UINT32_C(1) << CHAR_BREAK_PROP_EXTEND |   /* GB9  */
		UINT32_C(1)
			<< CHAR_BREAK_PROP_BOTH_EXTEND_ICB_EXTEND | /* GB9  */
		UINT32_C(1)
			<< CHAR_BREAK_PROP_BOTH_EXTEND_ICB_LINKER |  /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_ZWJ |                 /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_BOTH_ZWJ_ICB_EXTEND | /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_SPACINGMARK,          /* GB9a */
	[CHAR_BREAK_PROP_HANGUL_LVT] =
		UINT32_C(1) << CHAR_BREAK_PROP_HANGUL_T | /* GB8  */
		UINT32_C(1) << CHAR_BREAK_PROP_EXTEND |   /* GB9  */
		UINT32_C(1)
			<< CHAR_BREAK_PROP_BOTH_EXTEND_ICB_EXTEND | /* GB9  */
		UINT32_C(1)
			<< CHAR_BREAK_PROP_BOTH_EXTEND_ICB_LINKER |  /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_ZWJ |                 /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_BOTH_ZWJ_ICB_EXTEND | /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_SPACINGMARK,          /* GB9a */
	[CHAR_BREAK_PROP_PREPEND] =
		UINT32_C(1) << CHAR_BREAK_PROP_EXTEND | /* GB9  */
		UINT32_C(1)
			<< CHAR_BREAK_PROP_BOTH_EXTEND_ICB_EXTEND | /* GB9  */
		UINT32_C(1)
			<< CHAR_BREAK_PROP_BOTH_EXTEND_ICB_LINKER |  /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_ZWJ |                 /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_BOTH_ZWJ_ICB_EXTEND | /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_SPACINGMARK |         /* GB9a */
		(UINT32_C(0xFFFFFFFF) &
	         ~(UINT32_C(1) << CHAR_BREAK_PROP_CR |
	           UINT32_C(1) << CHAR_BREAK_PROP_LF |
	           UINT32_C(1) << CHAR_BREAK_PROP_CONTROL)), /* GB9b */
	[CHAR_BREAK_PROP_REGIONAL_INDICATOR] =
		UINT32_C(1) << CHAR_BREAK_PROP_EXTEND | /* GB9  */
		UINT32_C(1)
			<< CHAR_BREAK_PROP_BOTH_EXTEND_ICB_EXTEND | /* GB9  */
		UINT32_C(1)
			<< CHAR_BREAK_PROP_BOTH_EXTEND_ICB_LINKER |  /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_ZWJ |                 /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_BOTH_ZWJ_ICB_EXTEND | /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_SPACINGMARK,          /* GB9a */
	[CHAR_BREAK_PROP_SPACINGMARK] =
		UINT32_C(1) << CHAR_BREAK_PROP_EXTEND | /* GB9  */
		UINT32_C(1)
			<< CHAR_BREAK_PROP_BOTH_EXTEND_ICB_EXTEND | /* GB9  */
		UINT32_C(1)
			<< CHAR_BREAK_PROP_BOTH_EXTEND_ICB_LINKER |  /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_ZWJ |                 /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_BOTH_ZWJ_ICB_EXTEND | /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_SPACINGMARK,          /* GB9a */
	[CHAR_BREAK_PROP_ZWJ] =
		UINT32_C(1) << CHAR_BREAK_PROP_EXTEND | /* GB9  */
		UINT32_C(1)
			<< CHAR_BREAK_PROP_BOTH_EXTEND_ICB_EXTEND | /* GB9  */
		UINT32_C(1)
			<< CHAR_BREAK_PROP_BOTH_EXTEND_ICB_LINKER |  /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_ZWJ |                 /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_BOTH_ZWJ_ICB_EXTEND | /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_SPACINGMARK,          /* GB9a */
	[CHAR_BREAK_PROP_BOTH_ZWJ_ICB_EXTEND] =
		UINT32_C(1) << CHAR_BREAK_PROP_EXTEND | /* GB9  */
		UINT32_C(1)
			<< CHAR_BREAK_PROP_BOTH_EXTEND_ICB_EXTEND | /* GB9  */
		UINT32_C(1)
			<< CHAR_BREAK_PROP_BOTH_EXTEND_ICB_LINKER |  /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_ZWJ |                 /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_BOTH_ZWJ_ICB_EXTEND | /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_SPACINGMARK,          /* GB9a */

};
static const uint_least32_t flag_update_gb11[2 * NUM_CHAR_BREAK_PROPS] = {
	[CHAR_BREAK_PROP_EXTENDED_PICTOGRAPHIC] =
		UINT32_C(1) << CHAR_BREAK_PROP_ZWJ |
		UINT32_C(1) << CHAR_BREAK_PROP_BOTH_ZWJ_ICB_EXTEND | /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_EXTEND |              /* GB9  */
		UINT32_C(1)
			<< CHAR_BREAK_PROP_BOTH_EXTEND_ICB_EXTEND | /* GB9  */
		UINT32_C(1) << CHAR_BREAK_PROP_BOTH_EXTEND_ICB_LINKER, /* GB9 */
	[CHAR_BREAK_PROP_ZWJ + NUM_CHAR_BREAK_PROPS] =
		UINT32_C(1) << CHAR_BREAK_PROP_EXTENDED_PICTOGRAPHIC,
	[CHAR_BREAK_PROP_BOTH_ZWJ_ICB_EXTEND + NUM_CHAR_BREAK_PROPS] =
		UINT32_C(1) << CHAR_BREAK_PROP_EXTENDED_PICTOGRAPHIC,
	[CHAR_BREAK_PROP_EXTEND + NUM_CHAR_BREAK_PROPS] =
		UINT32_C(1) << CHAR_BREAK_PROP_EXTEND |
		UINT32_C(1) << CHAR_BREAK_PROP_BOTH_EXTEND_ICB_EXTEND |
		UINT32_C(1) << CHAR_BREAK_PROP_BOTH_EXTEND_ICB_LINKER |
		UINT32_C(1) << CHAR_BREAK_PROP_ZWJ |
		UINT32_C(1) << CHAR_BREAK_PROP_BOTH_ZWJ_ICB_EXTEND,
	[CHAR_BREAK_PROP_BOTH_EXTEND_ICB_EXTEND + NUM_CHAR_BREAK_PROPS] =
		UINT32_C(1) << CHAR_BREAK_PROP_EXTEND |
		UINT32_C(1) << CHAR_BREAK_PROP_BOTH_EXTEND_ICB_EXTEND |
		UINT32_C(1) << CHAR_BREAK_PROP_BOTH_EXTEND_ICB_LINKER |
		UINT32_C(1) << CHAR_BREAK_PROP_ZWJ |
		UINT32_C(1) << CHAR_BREAK_PROP_BOTH_ZWJ_ICB_EXTEND,
	[CHAR_BREAK_PROP_BOTH_EXTEND_ICB_LINKER + NUM_CHAR_BREAK_PROPS] =
		UINT32_C(1) << CHAR_BREAK_PROP_EXTEND |
		UINT32_C(1) << CHAR_BREAK_PROP_BOTH_EXTEND_ICB_EXTEND |
		UINT32_C(1) << CHAR_BREAK_PROP_BOTH_EXTEND_ICB_LINKER |
		UINT32_C(1) << CHAR_BREAK_PROP_ZWJ |
		UINT32_C(1) << CHAR_BREAK_PROP_BOTH_ZWJ_ICB_EXTEND,
	[CHAR_BREAK_PROP_EXTENDED_PICTOGRAPHIC + NUM_CHAR_BREAK_PROPS] =
		UINT32_C(1) << CHAR_BREAK_PROP_ZWJ |
		UINT32_C(1) << CHAR_BREAK_PROP_BOTH_ZWJ_ICB_EXTEND |
		UINT32_C(1) << CHAR_BREAK_PROP_EXTEND |
		UINT32_C(1) << CHAR_BREAK_PROP_BOTH_EXTEND_ICB_EXTEND |
		UINT32_C(1) << CHAR_BREAK_PROP_BOTH_EXTEND_ICB_LINKER,
};
static const uint_least32_t dont_break_gb11[2 * NUM_CHAR_BREAK_PROPS] = {
	[CHAR_BREAK_PROP_ZWJ + NUM_CHAR_BREAK_PROPS] =
		UINT32_C(1) << CHAR_BREAK_PROP_EXTENDED_PICTOGRAPHIC,
	[CHAR_BREAK_PROP_BOTH_ZWJ_ICB_EXTEND + NUM_CHAR_BREAK_PROPS] =
		UINT32_C(1) << CHAR_BREAK_PROP_EXTENDED_PICTOGRAPHIC,
};
static const uint_least32_t flag_update_gb12_13[2 * NUM_CHAR_BREAK_PROPS] = {
	[CHAR_BREAK_PROP_REGIONAL_INDICATOR] =
		UINT32_C(1) << CHAR_BREAK_PROP_REGIONAL_INDICATOR,
};
static const uint_least32_t dont_break_gb12_13[2 * NUM_CHAR_BREAK_PROPS] = {
	[CHAR_BREAK_PROP_REGIONAL_INDICATOR + NUM_CHAR_BREAK_PROPS] =
		UINT32_C(1) << CHAR_BREAK_PROP_REGIONAL_INDICATOR,
};

static inline enum char_break_property
get_break_prop(uint_least32_t cp)
{
	if (likely(cp <= UINT32_C(0x10FFFF))) {
		return (enum char_break_property)
			char_break_minor[char_break_major[cp >> 8] +
		                         (cp & 0xFF)];
	} else {
		return CHAR_BREAK_PROP_OTHER;
	}
}

static inline void
state_serialize(const struct character_break_state *in, uint_least16_t *out)
{
	*out = (uint_least16_t)(in->prop & UINT8_C(0xFF)) | /* first 8 bits */
	       (uint_least16_t)(((uint_least16_t)(in->prop_set))
	                        << 8) | /* 9th bit */
	       (uint_least16_t)(((uint_least16_t)(in->gb11_flag))
	                        << 9) | /* 10th bit */
	       (uint_least16_t)(((uint_least16_t)(in->gb12_13_flag))
	                        << 10) | /* 11th bit */
	       (uint_least16_t)(((uint_least16_t)(in->gb9c_level & 0x3))
	                        << 11); /* 12th and 13th bit */
}

static inline void
state_deserialize(uint_least16_t in, struct character_break_state *out)
{
	out->prop = in & UINT8_C(0xFF);
	out->prop_set = in & (UINT16_C(1) << 8);
	out->gb11_flag = in & (UINT16_C(1) << 9);
	out->gb12_13_flag = in & (UINT16_C(1) << 10);
	out->gb9c_level = (uint_least8_t)(in >> 11) & UINT8_C(0x3);
}

bool
grapheme_is_character_break(uint_least32_t cp0, uint_least32_t cp1,
                            uint_least16_t *s)
{
	struct character_break_state state;
	enum char_break_property cp0_prop, cp1_prop;
	bool notbreak = false;

	if (likely(s)) {
		state_deserialize(*s, &state);

		if (likely(state.prop_set)) {
			cp0_prop = state.prop;
		} else {
			cp0_prop = get_break_prop(cp0);
		}
		cp1_prop = get_break_prop(cp1);

		/* preserve prop of right codepoint for next iteration */
		state.prop = (uint_least8_t)cp1_prop;
		state.prop_set = true;

		/* update flags */
		state.gb11_flag =
			flag_update_gb11[cp0_prop + NUM_CHAR_BREAK_PROPS *
		                                            state.gb11_flag] &
			UINT32_C(1) << cp1_prop;
		state.gb12_13_flag =
			flag_update_gb12_13[cp0_prop +
		                            NUM_CHAR_BREAK_PROPS *
		                                    state.gb12_13_flag] &
			UINT32_C(1) << cp1_prop;

		/*
		 * update GB9c state, which deals with indic conjunct breaks.
		 * We want to detect the following prefix:
		 *
		 *   ICB_CONSONANT
		 *   [ICB_EXTEND ICB_LINKER]*
		 *   ICB_LINKER
		 *   [ICB_EXTEND ICB_LINKER]*
		 *
		 * This representation is not ideal: In reality, what is
		 * meant is that the prefix is a sequence of [ICB_EXTEND
		 * ICB_LINKER]*, following an ICB_CONSONANT, that contains at
		 * least one ICB_LINKER. We thus use the following equivalent
		 * representation that allows us to store the levels 0..3 in 2
		 * bits.
		 *
		 *   ICB_CONSONANT              -- Level 1
		 *   ICB_EXTEND*                -- Level 2
		 *   ICB_LINKER                 -- Level 3
		 *   [ICB_EXTEND ICB_LINKER]*   -- Level 3
		 *
		 * The following chain of if-else-blocks is a bit redundant and
		 * of course could be optimised, but this is kept as is for
		 * best readability.
		 */
		if (state.gb9c_level == 0 &&
		    cp0_prop == CHAR_BREAK_PROP_ICB_CONSONANT) {
			/* the sequence has begun */
			state.gb9c_level = 1;
		} else if ((state.gb9c_level == 1 || state.gb9c_level == 2) &&
		           (cp0_prop == CHAR_BREAK_PROP_ICB_EXTEND ||
		            cp0_prop == CHAR_BREAK_PROP_BOTH_ZWJ_ICB_EXTEND ||
		            cp0_prop ==
		                    CHAR_BREAK_PROP_BOTH_EXTEND_ICB_EXTEND)) {
			/*
			 * either the level is 1 and thus the ICB consonant is
			 * followed by an ICB extend, where we jump
			 * to level 2, or we are at level 2 and just witness
			 * more ICB extends, staying at level 2.
			 */
			state.gb9c_level = 2;
		} else if ((state.gb9c_level == 1 || state.gb9c_level == 2) &&
		           (cp0_prop == CHAR_BREAK_PROP_ICB_LINKER ||
		            cp0_prop ==
		                    CHAR_BREAK_PROP_BOTH_EXTEND_ICB_LINKER)) {
			/*
			 * witnessing an ICB linker directly lifts us up to
			 * level 3
			 */
			state.gb9c_level = 3;
		} else if (state.gb9c_level == 3 &&
		           (cp0_prop == CHAR_BREAK_PROP_ICB_EXTEND ||
		            cp0_prop == CHAR_BREAK_PROP_BOTH_ZWJ_ICB_EXTEND ||
		            cp0_prop ==
		                    CHAR_BREAK_PROP_BOTH_EXTEND_ICB_EXTEND ||
		            cp0_prop == CHAR_BREAK_PROP_ICB_LINKER ||
		            cp0_prop ==
		                    CHAR_BREAK_PROP_BOTH_EXTEND_ICB_LINKER)) {
			/*
			 * we stay at level 3 when we observe either ICB
			 * extends or linkers
			 */
			state.gb9c_level = 3;
		} else {
			/*
			 * the sequence has collapsed, but it could be
			 * that the left property is ICB consonant, which
			 * means that we jump right back to level 1 instead
			 * of 0
			 */
			if (cp0_prop == CHAR_BREAK_PROP_ICB_CONSONANT) {
				state.gb9c_level = 1;
			} else {
				state.gb9c_level = 0;
			}
		}

		/*
		 * Apply grapheme cluster breaking algorithm (UAX #29), see
		 * http://unicode.org/reports/tr29/#Grapheme_Cluster_Boundary_Rules
		 */
		notbreak = (dont_break[cp0_prop] & (UINT32_C(1) << cp1_prop)) ||
		           (state.gb9c_level == 3 &&
		            cp1_prop == CHAR_BREAK_PROP_ICB_CONSONANT) ||
		           (dont_break_gb11[cp0_prop +
		                            state.gb11_flag *
		                                    NUM_CHAR_BREAK_PROPS] &
		            (UINT32_C(1) << cp1_prop)) ||
		           (dont_break_gb12_13[cp0_prop +
		                               state.gb12_13_flag *
		                                       NUM_CHAR_BREAK_PROPS] &
		            (UINT32_C(1) << cp1_prop));

		/* update or reset flags (when we have a break) */
		if (likely(!notbreak)) {
			state.gb11_flag = state.gb12_13_flag = false;
		}

		state_serialize(&state, s);
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
		notbreak = (dont_break[cp0_prop] & (UINT32_C(1) << cp1_prop)) ||
		           (dont_break_gb11[cp0_prop] &
		            (UINT32_C(1) << cp1_prop)) ||
		           (dont_break_gb12_13[cp0_prop] &
		            (UINT32_C(1) << cp1_prop));
	}

	return !notbreak;
}

static size_t
next_character_break(HERODOTUS_READER *r)
{
	uint_least16_t state = 0;
	uint_least32_t cp0 = 0, cp1 = 0;

	for (herodotus_read_codepoint(r, true, &cp0);
	     herodotus_read_codepoint(r, false, &cp1) ==
	     HERODOTUS_STATUS_SUCCESS;
	     herodotus_read_codepoint(r, true, &cp0)) {
		if (grapheme_is_character_break(cp0, cp1, &state)) {
			break;
		}
	}

	return herodotus_reader_number_read(r);
}

size_t
grapheme_next_character_break(const uint_least32_t *str, size_t len)
{
	HERODOTUS_READER r;

	herodotus_reader_init(&r, HERODOTUS_TYPE_CODEPOINT, str, len);

	return next_character_break(&r);
}

size_t
grapheme_next_character_break_utf8(const char *str, size_t len)
{
	HERODOTUS_READER r;

	herodotus_reader_init(&r, HERODOTUS_TYPE_UTF8, str, len);

	return next_character_break(&r);
}
