/* See LICENSE file for copyright and license details. */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define LEN(x) (sizeof(x) / sizeof(*x))

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

enum property {
	PROP_CR,           /* carriage return */
	PROP_LF,           /* line feed */
	PROP_CONTROL,      /* control character */
	PROP_EXTEND,       /* grapheme extender (TODO Emoji_Modifier=Yes) */
	PROP_ZWJ,          /* zero width joiner */
	PROP_RI,           /* regional indicator */
	PROP_PREPEND,      /* prepend character */
	PROP_SPACINGMARK,  /* spacing mark */
	PROP_L,            /* hangul syllable type L */
	PROP_V,            /* hangul syllable type V */
	PROP_T,            /* hangul syllable type T */
	PROP_LV,           /* hangul syllable type LV */
	PROP_LVT,          /* hangul syllable type LVT */
	PROP_EXTPICT,      /* extended pictographic */
	NUM_PROPS,
};

struct {
	const uint32_t (*table)[2];
	size_t tablelen;
} tables[] = {
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

static int
is(uint32_t cp[2], char (*props)[2], int index, enum property p)
{
	if (props[p][index] == 2) {
		/* need to determine property */
		props[p][index] = (bsearch(&(cp[index]),
			tables[p].table,
			tables[p].tablelen,
			sizeof(*(tables[p].table)),
			cp_cmp) == NULL) ? 0 : 1;
	}

	return props[p][index];
}

#define IS(I, PROP) (is(cp, props, I, PROP))

int
grapheme_boundary(uint32_t cp0, uint32_t cp1, int *state)
{
	uint32_t cp[2] = { cp0, cp1 };
	char props[NUM_PROPS][2];
	size_t i;

	if ((cp0 >= 0x20 && cp0 <= 0x7E) &&
	    (cp1 >= 0x20 && cp1 <= 0x7E)) {
		/* skip printable ascii */
		return 1;
	}

	/* set all properties to undetermined (2) */
	for (i = 0; i < NUM_PROPS; i++) {
		props[i][0] = props[i][1] = 2;
	}

	/* http://unicode.org/reports/tr29/#Grapheme_Cluster_Boundary_Rules */

	/* update state machine */
	if (IS(1, PROP_RI)) {
		if (IS(0, PROP_RI)) {
			/* one more RI is on the left side of the seam */
			*state ^= GRAPHEME_STATE_RI_ODD;
		} else {
			/* an RI appeared on the right side but the left
			   side is not an RI, reset state (0 is even) */
			*state &= ~GRAPHEME_STATE_RI_ODD;
		}
	}
	if (!(*state & GRAPHEME_STATE_EMOJI) &&
	    ((IS(0, PROP_EXTPICT) && IS(1, PROP_ZWJ)) ||
             (IS(0, PROP_EXTPICT) && IS(1, PROP_EXTEND)))) {
		*state |= GRAPHEME_STATE_EMOJI;
	} else if ((*state & GRAPHEME_STATE_EMOJI) &&
	           (
	            (IS(0, PROP_ZWJ)     && IS(1, PROP_EXTPICT)) ||
	            (IS(0, PROP_EXTEND)  && IS(1, PROP_EXTEND)) ||
	            (IS(0, PROP_EXTEND)  && IS(1, PROP_ZWJ)) ||
	            (IS(0, PROP_EXTPICT) && IS(1, PROP_ZWJ)) ||
	            (IS(0, PROP_EXTPICT) && IS(1, PROP_EXTEND))
		   )
	          ) {
		/* GRAPHEME_STATE_EMOJI remains */
	} else {
		*state &= ~GRAPHEME_STATE_EMOJI;
	}

	/* GB3 */
	if (IS(0, PROP_CR) && IS(1, PROP_LF)) {
		return 0;
	}

	/* GB4 */
	if (IS(0, PROP_CONTROL) || IS(0, PROP_CR) || IS(0, PROP_LF)) {
		return 1;
	}

	/* GB5 */
	if (IS(1, PROP_CONTROL) || IS(1, PROP_CR) || IS(1, PROP_LF)) {
		return 1;
	}

	/* GB6 */
	if (IS(0, PROP_L) && (IS(1, PROP_L)  || IS(1, PROP_V) ||
	                      IS(1, PROP_LV) || IS(1, PROP_LVT))) {
		return 0;
	}

	/* GB7 */
	if ((IS(0, PROP_LV) || IS(0, PROP_V)) && (IS(1, PROP_V) ||
	                                          IS(1, PROP_T))) {
		return 0;
	}

	/* GB8 */
	if ((IS(0, PROP_LVT) || IS(0, PROP_T)) && IS(1, PROP_T)) {
		return 0;
	}

	/* GB9 */
	if (IS(1, PROP_EXTEND) || IS(1, PROP_ZWJ)) {
		return 0;
	}

	/* GB9a */
	if (IS(1, PROP_SPACINGMARK)) {
		return 0;
	}

	/* GB9b */
	if (IS(0, PROP_PREPEND)) {
		return 0;
	}

	/* GB11 */
	if ((*state & GRAPHEME_STATE_EMOJI) && IS(0, PROP_ZWJ) &&
	    IS(1, PROP_EXTPICT)) {
		return 0;
	}

	/* GB12/GB13 */
	if (IS(0, PROP_RI) && IS(1, PROP_RI) &&
	    (*state & GRAPHEME_STATE_RI_ODD)) {
		return 0;
	}

	/* GB999 */
	return 1;
}
