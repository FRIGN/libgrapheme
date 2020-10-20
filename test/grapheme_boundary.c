/* See LICENSE file for copyright and license details. */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../grapheme.h"
#include "../data/grapheme_boundary_test.h"

#define LEN(x) (sizeof(x) / sizeof(*x))

int
main(void)
{
	int state;
	size_t i, j, k, len, failed;

	/* grapheme break test */
	for (i = 0, failed = 0; i < LEN(t); i++) {
		for (j = 0, k = 0, state = 0, len = 1; j < t[i].cplen; j++) {
			if ((j + 1) == t[i].cplen ||
			    grapheme_boundary(t[i].cp[j], t[i].cp[j + 1],
			                      &state)) {
				/* check if our resulting length matches */
				if (k == t[i].lenlen || len != t[i].len[k++]) {
					fprintf(stderr, "Failed \"%s\"\n",
					        t[i].descr);
					failed++;
					break;
				}
				len = 1;
			} else {
				len++;
			}
		}
	}
	printf("Grapheme break test: Passed %zu out of %zu tests.\n",
	       LEN(t) - failed, LEN(t));

	return (failed > 0) ? 1 : 0;
}
