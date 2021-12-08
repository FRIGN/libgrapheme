/* See LICENSE file for copyright and license details. */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../grapheme.h"
#include "../gen/grapheme-test.h"

#define LEN(x) (sizeof(x) / sizeof(*x))

int
main(void)
{
	int state;
	size_t i, j, k, len, failed;

	/* grapheme break test */
	for (i = 0, failed = 0; i < LEN(grapheme_test); i++) {
		for (j = 0, k = 0, state = 0, len = 1; j < grapheme_test[i].cplen; j++) {
			if ((j + 1) == grapheme_test[i].cplen ||
			    grapheme_boundary(grapheme_test[i].cp[j],
			                      grapheme_test[i].cp[j + 1],
			                      &state)) {
				/* check if our resulting length matches */
				if (k == grapheme_test[i].lenlen ||
				    len != grapheme_test[i].len[k++]) {
					fprintf(stderr, "Failed \"%s\"\n",
					        grapheme_test[i].descr);
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
	       LEN(grapheme_test) - failed, LEN(grapheme_test));

	return (failed > 0) ? 1 : 0;
}
