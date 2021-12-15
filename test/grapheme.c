/* See LICENSE file for copyright and license details. */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../grapheme.h"
#include "../gen/grapheme-test.h"
#include "util.h"

int
main(int argc, char *argv[])
{
	LG_SEGMENTATION_STATE state;
	size_t i, j, k, len, failed;

	(void)argc;

	/* grapheme break test */
	for (i = 0, failed = 0; i < LEN(grapheme_test); i++) {
		memset(&state, 0, sizeof(state));
		for (j = 0, k = 0, len = 1; j < grapheme_test[i].cplen; j++) {
			if ((j + 1) == grapheme_test[i].cplen ||
			    lg_grapheme_isbreak(grapheme_test[i].cp[j],
			                        grapheme_test[i].cp[j + 1],
			                        &state)) {
				/* check if our resulting length matches */
				if (k == grapheme_test[i].lenlen ||
				    len != grapheme_test[i].len[k++]) {
					fprintf(stderr, "%s: Failed test \"%s\".\n",
					        argv[0], grapheme_test[i].descr);
					failed++;
					break;
				}
				len = 1;
			} else {
				len++;
			}
		}
	}
	printf("%s: %zu/%zu tests passed.\n", argv[0],
	       LEN(grapheme_test) - failed, LEN(grapheme_test));

	return (failed > 0) ? 1 : 0;
}
