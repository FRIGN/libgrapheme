/* See LICENSE file for copyright and license details. */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "../data/emo.h"
#include "../data/gbp.h"

#define LEN(x) (sizeof(x) / sizeof(*x))

enum {
	GRAPHEME_STATE_RI_ODD = 1 << 0, /* odd number of RI's before the seam */
	GRAPHEME_STATE_EMOJI  = 1 << 1, /* within emoji modifier or zwj sequence */
};

enum cp_property {
	PROP_CR,          /* carriage return */
	PROP_LF,          /* line feed */
	PROP_CONTROL,     /* control character */
	PROP_EXTEND,      /* grapheme extender (TODO Emoji_Modifier=Yes) */
	PROP_ZWJ,         /* zero width joiner */
	PROP_RI,          /* regional indicator */
	PROP_PREPEND,     /* prepend character */
	PROP_SPACINGMARK, /* spacing mark */
	PROP_L,           /* hangul syllable type L */
	PROP_V,           /* hangul syllable type V */
	PROP_T,           /* hangul syllable type T */
	PROP_LV,          /* hangul syllable type LV */
	PROP_LVT,         /* hangul syllable type LVT */
	PROP_EXTPICT,     /* extended pictographic */
};

struct {
	const uint32_t (*table)[2];
	size_t tablelen;
} cp_property_tables[] = {
	[PROP_CR] = {
		.table    = cr_table,
		.tablelen = LEN(cr_table),
	},
	[PROP_LF] = {
		.table    = lf_table,
		.tablelen = LEN(lf_table),
	},
	[PROP_CONTROL] = {
		.table    = control_table,
		.tablelen = LEN(control_table),
	},
	[PROP_EXTEND] = {
		.table    = extend_table,
		.tablelen = LEN(extend_table),
	},
	[PROP_ZWJ] = {
		.table    = zwj_table,
		.tablelen = LEN(zwj_table),
	},
	[PROP_RI] = {
		.table    = ri_table,
		.tablelen = LEN(ri_table),
	},
	[PROP_PREPEND] = {
		.table    = prepend_table,
		.tablelen = LEN(prepend_table),
	},
	[PROP_SPACINGMARK] = {
		.table    = spacingmark_table,
		.tablelen = LEN(spacingmark_table),
	},
	[PROP_L] = {
		.table    = l_table,
		.tablelen = LEN(l_table),
	},
	[PROP_V] = {
		.table    = v_table,
		.tablelen = LEN(v_table),
	},
	[PROP_T] = {
		.table    = t_table,
		.tablelen = LEN(t_table),
	},
	[PROP_LV] = {
		.table    = lv_table,
		.tablelen = LEN(lv_table),
	},
	[PROP_LVT] = {
		.table    = lvt_table,
		.tablelen = LEN(lvt_table),
	},
	[PROP_EXTPICT] = {
		.table    = extpict_table,
		.tablelen = LEN(extpict_table),
	},
};

struct cp_properties {
	uint32_t cp;
	int_least16_t determined;
	int_least16_t state;
};

static int
cp_cmp(const void *a, const void *b)
{
	uint32_t cp = *(uint32_t *)a;
	uint32_t *range = (uint32_t *)b;

	return (cp >= range[0] && cp <= range[1]) ? 0 : (cp - range[0]);
}

static int
has_property(struct cp_properties *props, enum cp_property p)
{
	if (!(props->determined & (1 << p))) {
		/* not determined yet, do a lookup and set the state */
		if (bsearch(&props->cp, cp_property_tables[p].table,
		            cp_property_tables[p].tablelen,
		            sizeof(*cp_property_tables[p].table),
			    cp_cmp)) {
			props->state |= (1 << p);
		} else {
			props->state &= ~(1 << p);
		}

		/* now it's determined */
		props->determined |= (1 << p);
	}

	return (props->state & (1 << p));
}

int
grapheme_boundary(uint32_t a, uint32_t b, int *state)
{
	struct cp_properties props[] = {
		{
			.cp = a,
		},
		{
			.cp = b,
		},
	};
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
	if (has_property(&props[1], PROP_RI)) {
		if (has_property(&props[0], PROP_RI)) {
			/* one more RI is on the left side of the seam */
			s ^= GRAPHEME_STATE_RI_ODD;
		} else {
			/* an RI appeared on the right side but the left
			   side is not an RI, reset state (0 is even) */
			s &= ~GRAPHEME_STATE_RI_ODD;
		}
	}
	if (!(*state & GRAPHEME_STATE_EMOJI) &&
	    ((has_property(&props[0], PROP_EXTPICT) &&
	      has_property(&props[1], PROP_ZWJ)) ||
             (has_property(&props[0], PROP_EXTPICT) &&
	      has_property(&props[1], PROP_EXTEND)))) {
		s |= GRAPHEME_STATE_EMOJI;
	} else if ((*state & GRAPHEME_STATE_EMOJI) &&
	           ((has_property(&props[0], PROP_ZWJ) &&
		     has_property(&props[1], PROP_EXTPICT)) ||
	            (has_property(&props[0], PROP_EXTEND) &&
		     has_property(&props[1], PROP_EXTEND)) ||
	            (has_property(&props[0], PROP_EXTEND) &&
		     has_property(&props[1], PROP_ZWJ)) ||
	            (has_property(&props[0], PROP_EXTPICT) &&
		     has_property(&props[1], PROP_ZWJ)) ||
	            (has_property(&props[0], PROP_EXTPICT) &&
		     has_property(&props[1], PROP_EXTEND)))) {
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
	if (has_property(&props[0], PROP_CR) &&
	    has_property(&props[1], PROP_LF)) {
		return 0;
	}

	/* GB4 */
	if (has_property(&props[0], PROP_CONTROL) ||
	    has_property(&props[0], PROP_CR) ||
	    has_property(&props[0], PROP_LF)) {
		return 1;
	}

	/* GB5 */
	if (has_property(&props[1], PROP_CONTROL) ||
	    has_property(&props[1], PROP_CR) ||
	    has_property(&props[1], PROP_LF)) {
		return 1;
	}

	/* GB6 */
	if (has_property(&props[0], PROP_L) &&
	    (has_property(&props[1], PROP_L) ||
	     has_property(&props[1], PROP_V) ||
	     has_property(&props[1], PROP_LV) ||
	     has_property(&props[1], PROP_LVT))) {
		return 0;
	}

	/* GB7 */
	if ((has_property(&props[0], PROP_LV) ||
	     has_property(&props[0], PROP_V)) &&
	    (has_property(&props[1], PROP_V) ||
	     has_property(&props[1], PROP_T))) {
		return 0;
	}

	/* GB8 */
	if ((has_property(&props[0], PROP_LVT) ||
	     has_property(&props[0], PROP_T)) &&
	    has_property(&props[1], PROP_T)) {
		return 0;
	}

	/* GB9 */
	if (has_property(&props[1], PROP_EXTEND) ||
	    has_property(&props[1], PROP_ZWJ)) {
		return 0;
	}

	/* GB9a */
	if (has_property(&props[1], PROP_SPACINGMARK)) {
		return 0;
	}

	/* GB9b */
	if (has_property(&props[0], PROP_PREPEND)) {
		return 0;
	}

	/* GB11 */
	if ((s & GRAPHEME_STATE_EMOJI) &&
	    has_property(&props[0], PROP_ZWJ) &&
	    has_property(&props[1], PROP_EXTPICT)) {
		return 0;
	}

	/* GB12/GB13 */
	if (has_property(&props[0], PROP_RI) &&
	    has_property(&props[1], PROP_RI) &&
	    (s & GRAPHEME_STATE_RI_ODD)) {
		return 0;
	}

	/* GB999 */
	return 1;
}
