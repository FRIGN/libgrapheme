/* See LICENSE file for copyright and license details. */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "../data/emoji.h"
#include "../data/grapheme_boundary.h"

enum {
	GRAPHEME_STATE_RI_ODD = 1 << 0, /* odd number of RI's before the seam */
	GRAPHEME_STATE_EMOJI  = 1 << 1, /* within emoji modifier or zwj sequence */
};

static int
cp_cmp(const void *a, const void *b)
{
	uint32_t cp = *(uint32_t *)a;
	uint32_t *range = (uint32_t *)b;

	return (cp >= range[0] && cp <= range[1]) ? 0 : (cp - range[0]);
}

static int
has_property(uint32_t cp, struct heisenstate *cpstate,
             const struct range_list *proptable, int property)
{
	if (heisenstate_get(cpstate, property) == -1) {
		/* state undetermined, make a lookup and set it */
		heisenstate_set(cpstate, property, bsearch(&cp,
		                proptable[property].data,
		                proptable[property].len,
				sizeof(*proptable[property].data),
		                cp_cmp) ? 1 : 0);
	}

	return heisenstate_get(cpstate, property);
}

int
grapheme_boundary(uint32_t a, uint32_t b, int *state)
{
	struct heisenstate gb[2] = { 0 }, emoji[2] = { 0 };
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
	if (has_property(b, &gb[1], gb_prop, GB_PROP_REGIONAL_INDICATOR)) {
		if (has_property(a, &gb[0], gb_prop, GB_PROP_REGIONAL_INDICATOR)) {
			/* one more RI is on the left side of the seam */
			s ^= GRAPHEME_STATE_RI_ODD;
		} else {
			/* an RI appeared on the right side but the left
			   side is not an RI, reset state (0 is even) */
			s &= ~GRAPHEME_STATE_RI_ODD;
		}
	}
	if (!(*state & GRAPHEME_STATE_EMOJI) &&
	    ((has_property(a, &emoji[0], emoji_prop, EMOJI_PROP_EXTPICT) &&
	      has_property(b, &gb[1],    gb_prop,    GB_PROP_ZWJ)) ||
             (has_property(a, &emoji[0], emoji_prop, EMOJI_PROP_EXTPICT) &&
	      has_property(b, &gb[1],    gb_prop,    GB_PROP_EXTEND)))) {
		s |= GRAPHEME_STATE_EMOJI;
	} else if ((*state & GRAPHEME_STATE_EMOJI) &&
	           ((has_property(a, &gb[0],    gb_prop,    GB_PROP_ZWJ) &&
		     has_property(b, &emoji[1], emoji_prop, EMOJI_PROP_EXTPICT)) ||
	            (has_property(a, &gb[0],    gb_prop,    GB_PROP_EXTEND) &&
		     has_property(b, &gb[1],    gb_prop,    GB_PROP_EXTEND)) ||
	            (has_property(a, &gb[0],    gb_prop,    GB_PROP_EXTEND) &&
		     has_property(b, &gb[1],    gb_prop,    GB_PROP_ZWJ)) ||
	            (has_property(a, &emoji[0], emoji_prop, EMOJI_PROP_EXTPICT) &&
		     has_property(b, &gb[1],    gb_prop,    GB_PROP_ZWJ)) ||
	            (has_property(a, &emoji[0], emoji_prop, EMOJI_PROP_EXTPICT) &&
		     has_property(b, &gb[1],    gb_prop,    GB_PROP_EXTEND)))) {
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
	if (has_property(a, &gb[0], gb_prop, GB_PROP_CR) &&
	    has_property(b, &gb[1], gb_prop, GB_PROP_LF)) {
		return 0;
	}

	/* GB4 */
	if (has_property(a, &gb[0], gb_prop, GB_PROP_CONTROL) ||
	    has_property(a, &gb[0], gb_prop, GB_PROP_CR) ||
	    has_property(a, &gb[0], gb_prop, GB_PROP_LF)) {
		return 1;
	}

	/* GB5 */
	if (has_property(b, &gb[1], gb_prop, GB_PROP_CONTROL) ||
	    has_property(b, &gb[1], gb_prop, GB_PROP_CR) ||
	    has_property(b, &gb[1], gb_prop, GB_PROP_LF)) {
		return 1;
	}

	/* GB6 */
	if (has_property(a, &gb[0], gb_prop, GB_PROP_L) &&
	    (has_property(b, &gb[1], gb_prop, GB_PROP_L) ||
	     has_property(b, &gb[1], gb_prop, GB_PROP_V) ||
	     has_property(b, &gb[1], gb_prop, GB_PROP_LV) ||
	     has_property(b, &gb[1], gb_prop, GB_PROP_LVT))) {
		return 0;
	}

	/* GB7 */
	if ((has_property(a, &gb[0], gb_prop, GB_PROP_LV) ||
	     has_property(a, &gb[0], gb_prop, GB_PROP_V)) &&
	    (has_property(b, &gb[1], gb_prop, GB_PROP_V) ||
	     has_property(b, &gb[1], gb_prop, GB_PROP_T))) {
		return 0;
	}

	/* GB8 */
	if ((has_property(a, &gb[0], gb_prop, GB_PROP_LVT) ||
	     has_property(a, &gb[0], gb_prop, GB_PROP_T)) &&
	    has_property(b, &gb[1], gb_prop, GB_PROP_T)) {
		return 0;
	}

	/* GB9 */
	if (has_property(b, &gb[1], gb_prop, GB_PROP_EXTEND) ||
	    has_property(b, &gb[1], gb_prop, GB_PROP_ZWJ)) {
		return 0;
	}

	/* GB9a */
	if (has_property(b, &gb[1], gb_prop, GB_PROP_SPACINGMARK)) {
		return 0;
	}

	/* GB9b */
	if (has_property(a, &gb[0], gb_prop, GB_PROP_PREPEND)) {
		return 0;
	}

	/* GB11 */
	if ((s & GRAPHEME_STATE_EMOJI) &&
	    has_property(a, &gb[0], gb_prop, GB_PROP_ZWJ) &&
	    has_property(b, &emoji[1], emoji_prop, EMOJI_PROP_EXTPICT)) {
		return 0;
	}

	/* GB12/GB13 */
	if (has_property(a, &gb[0], gb_prop, GB_PROP_REGIONAL_INDICATOR) &&
	    has_property(b, &gb[1], gb_prop, GB_PROP_REGIONAL_INDICATOR) &&
	    (s & GRAPHEME_STATE_RI_ODD)) {
		return 0;
	}

	/* GB999 */
	return 1;
}
