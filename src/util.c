/* See LICENSE file for copyright and license details. */
#include <stdint.h>
#include <stdlib.h>

#include "util.h"

int
heisenstate_get(struct heisenstate *h, int slot)
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
heisenstate_set(struct heisenstate *h, int slot, int state)
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
	uint32_t cp = *(uint32_t *)a;
	uint32_t *range = (uint32_t *)b;

	return (cp >= range[0] && cp <= range[1]) ? 0 : (cp - range[0]);
}

int
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
