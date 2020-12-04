/* See LICENSE file for copyright and license details. */
#include "util.h"

int
heisenstate_get(struct heisenstate *h, int slot)
{
	if (h == NULL || slot >= 16 || slot < 0 ||
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
	if (h == NULL || slot >= 16 || slot < 0) {
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
