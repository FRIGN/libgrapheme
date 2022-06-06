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
run_break_tests(size_t (*next_break)(const uint_least32_t *, size_t),
                const struct break_test *test, size_t testlen, const char *argv0)
{
	size_t i, j, off, res, failed;

	/* character break test */
	for (i = 0, failed = 0; i < testlen; i++) {
		for (j = 0, off = 0; off < test[i].cplen; off += res) {
			res = next_break(test[i].cp + off, test[i].cplen - off);

			/* check if our resulting offset matches */
			if (j == test[i].lenlen ||
			    res != test[i].len[j++]) {
				fprintf(stderr, "%s: Failed test %zu \"%s\".\n",
				        argv0, i, test[i].descr);
				fprintf(stderr, "J=%zu: EXPECTED len %zu, got %zu\n", j-1, test[i].len[j-1], res);
				failed++;
				break;
			}
		}
	}
	printf("%s: %zu/%zu tests passed.\n", argv0,
	       testlen - failed, testlen);

	return (failed > 0) ? 1 : 0;
}
