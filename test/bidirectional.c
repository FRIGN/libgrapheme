/* See LICENSE file for copyright and license details. */
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "../gen/bidirectional-test.h"
#include "../gen/bidirectional.h"
#include "../gen/types.h"
#include "../grapheme.h"
#include "util.h"

static inline int_least16_t
get_mirror_offset(uint_least32_t cp)
{
	if (cp <= UINT32_C(0x10FFFF)) {
		return mirror_minor[mirror_major[cp >> 8] + (cp & 0xFF)];
	} else {
		return 0;
	}
}

int
main(int argc, char *argv[])
{
	enum grapheme_bidirectional_direction resolved;
	uint_least32_t data[512], output[512],
		target; /* TODO iterate and get max, allocate */
	int_least8_t lev[512];
	size_t i, num_tests, failed, datalen, levlen, outputlen, ret, j, m,
		ret2;

	datalen = LEN(data);
	levlen = LEN(lev);
	outputlen = LEN(output);

	(void)argc;

	for (i = 0, num_tests = 0; i < LEN(bidirectional_test); i++) {
		num_tests += bidirectional_test[i].modelen;
	}

	for (i = 0, failed = 0; i < LEN(bidirectional_test); i++) {
		for (m = 0; m < bidirectional_test[i].modelen; m++) {
			ret = grapheme_bidirectional_preprocess_paragraph(
				bidirectional_test[i].cp,
				bidirectional_test[i].cplen,
				bidirectional_test[i].mode[m], data, datalen,
				&resolved);
			ret2 = 0;

			if (ret != bidirectional_test[i].cplen ||
			    ret > datalen) {
				goto err;
			}

			/* resolved paragraph level (if specified in the test)
			 */
			if (bidirectional_test[i].resolved !=
			            GRAPHEME_BIDIRECTIONAL_DIRECTION_NEUTRAL &&
			    resolved != bidirectional_test[i].resolved) {
				goto err;
			}

			/* line levels */
			ret = grapheme_bidirectional_get_line_embedding_levels(
				data, ret, lev, levlen);

			if (ret > levlen) {
				goto err;
			}

			for (j = 0; j < ret; j++) {
				if (lev[j] != bidirectional_test[i].level[j]) {
					goto err;
				}
			}

			/* reordering */
			ret2 = grapheme_bidirectional_reorder_line(
				bidirectional_test[i].cp, data, ret, output,
				outputlen);

			if (ret2 != bidirectional_test[i].reorderlen) {
				goto err;
			}

			for (j = 0; j < ret2; j++) {
				target = bidirectional_test[i]
				                 .cp[bidirectional_test[i]
				                             .reorder[j]];
				if (output[j] !=
				    (uint_least32_t)((int_least32_t)target +
				                     get_mirror_offset(
							     target))) {
					goto err;
				}
			}

			continue;
err:
			fprintf(stderr,
			        "%s: Failed conformance test %zu (mode %i) [",
			        argv[0], i, bidirectional_test[i].mode[m]);
			for (j = 0; j < bidirectional_test[i].cplen; j++) {
				fprintf(stderr, " 0x%04" PRIXLEAST32,
				        bidirectional_test[i].cp[j]);
			}
			fprintf(stderr, " ],\n\tlevels: got      (");
			for (j = 0; j < ret; j++) {
				fprintf(stderr, " %" PRIdLEAST8,
				        (int_least8_t)lev[j]);
			}
			fprintf(stderr, " ),\n\tlevels: expected (");
			for (j = 0; j < ret; j++) {
				fprintf(stderr, " %" PRIdLEAST8,
				        bidirectional_test[i].level[j]);
			}
			fprintf(stderr, " ).\n");

			fprintf(stderr, "\treordering: got      (");
			for (j = 0; j < ret2; j++) {
				fprintf(stderr, " 0x%04" PRIxLEAST32,
				        output[j]);
			}
			fprintf(stderr, " ),\n\treordering: expected (");
			for (j = 0; j < bidirectional_test[i].reorderlen; j++) {
				fprintf(stderr, " 0x%04" PRIxLEAST32,
				        bidirectional_test[i]
				                .cp[bidirectional_test[i]
				                            .reorder[j]]);
			}
			fprintf(stderr, " ).\n");

			failed++;
		}
	}
	printf("%s: %zu/%zu conformance tests passed.\n", argv[0],
	       num_tests - failed, num_tests);

	return 0;
}
