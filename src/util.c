/* See LICENSE file for copyright and license details. */
#include <stdint.h>
#include <stdlib.h>

#include "../grapheme.h"
#include "util.h"

/* 64-slot (0,...,63) optionally undetermined binary state */

int
heisenstate_get(struct lg_internal_heisenstate *h, int slot)
{
	if (h == NULL || slot >= 64 || slot < 0 ||
	    !(h->determined & (1 << slot))) {
		/* no state given, slot out of range or undetermined */
		return -1;
	} else {
		/* slot determined, return state (0 or 1) */
		return (h->state & (1 << slot)) ? 1 : 0;
	}
}

int
heisenstate_set(struct lg_internal_heisenstate *h, int slot, int state)
{
	if (h == NULL || slot >= 64 || slot < 0) {
		/* no state given or slot out of range */
		return 1;
	} else {
		h->determined |= (1 << slot);
		if (state) {
			h->state |= (1 << slot);
		} else {
			h->state &= ~(1 << slot);
		}
	}

	return 0;
}

static int
cp_cmp(const void *a, const void *b)
{
	uint_least32_t cp = *(const uint_least32_t *)a;
	const uint_least32_t *range = (const uint_least32_t *)b;

	return (cp >= range[0] && cp <= range[1]) ? 0 : (int)(cp - range[0]);
}

int
has_property(uint_least32_t cp, struct lg_internal_heisenstate *cpstate,
             const struct range_list *proptable, int property)
{
	int res;

	if (cpstate == NULL ||
	    (res = heisenstate_get(cpstate, property)) == -1) {
		/* make a lookup */
		res = bsearch(&cp, proptable[property].data,
		              proptable[property].len,
		              sizeof(*proptable[property].data), cp_cmp) ?
		      1 : 0;

		if (cpstate != NULL) {
			heisenstate_set(cpstate, property, res);
		}
	}

	return res;
}
