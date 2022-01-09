/* See LICENSE file for copyright and license details. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../grapheme.h"
#include "../gen/types.h"
#include "util.h"

int
run_break_tests(bool (*is_break)(uint_least32_t, uint_least32_t, GRAPHEME_STATE *),
                const struct test *test, size_t testlen, const char *argv0)
{
	GRAPHEME_STATE state;
	size_t i, j, k, len, failed;

	/* character break test */
	for (i = 0, failed = 0; i < testlen; i++) {
		memset(&state, 0, sizeof(state));
		for (j = 0, k = 0, len = 1; j < test[i].cplen; j++) {
			if ((j + 1) == test[i].cplen ||
			    is_break(test[i].cp[j], test[i].cp[j + 1],
			             &state)) {
				/* check if our resulting length matches */
				if (k == test[i].lenlen ||
				    len != test[i].len[k++]) {
					fprintf(stderr, "%s: Failed test \"%s\".\n",
					        argv0, test[i].descr);
					failed++;
					break;
				}
				len = 1;
			} else {
				len++;
			}
		}
	}
	printf("%s: %zu/%zu tests passed.\n", argv0,
	       testlen - failed, testlen);

	return (failed > 0) ? 1 : 0;
}
