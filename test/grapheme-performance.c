/* See LICENSE file for copyright and license details. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../grapheme.h"
#include "../gen/grapheme-test.h"

#define LEN(x) (sizeof(x) / sizeof(*(x)))
#define NUM_ITERATIONS 10000

int64_t time_diff(struct timespec *a, struct timespec *b)
{
	return ((b->tv_sec * 1000000000) + b->tv_nsec) -
	       ((a->tv_sec * 1000000000) + a->tv_nsec);
}

int
main(void)
{
	struct timespec start, end;
	size_t i, j, bufsiz, off;
	uint32_t *buf;
	LG_SEGMENTATION_STATE state;
	double cp_per_sec;

	/* allocate and generate buffer */
	for (i = 0, bufsiz = 0; i < LEN(grapheme_test); i++) {
		bufsiz += grapheme_test[i].cplen;
	}
	if (!(buf = calloc(bufsiz, sizeof(*buf)))) {
		fprintf(stderr, "calloc: Out of memory.\n");
		return 1;
	}
	for (i = 0, off = 0; i < LEN(grapheme_test); i++) {
		for (j = 0; j < grapheme_test[i].cplen; j++) {
			buf[off + j] = grapheme_test[i].cp[j];
		}
		off += grapheme_test[i].cplen;
	}

	/* run test */
	printf("Grapheme break performance test: ");
	fflush(stdout);

	clock_gettime(CLOCK_MONOTONIC, &start);
	for (i = 0; i < NUM_ITERATIONS; i++) {
		memset(&state, 0, sizeof(state));
		for (j = 0; j < bufsiz - 1; j++) {
			(void)lg_grapheme_isbreak(buf[j], buf[j+1], &state);
		}
		if (i % (NUM_ITERATIONS / 10) == 0) {
			printf(".");
			fflush(stdout);
		}
	}
	clock_gettime(CLOCK_MONOTONIC, &end);
	cp_per_sec = ((double)NUM_ITERATIONS * bufsiz) /
	             ((double)time_diff(&start, &end) / 1000000000);

	printf(" %.2e CP/s\n", cp_per_sec);

	return 0;
}
