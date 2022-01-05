/* See LICENSE file for copyright and license details. */
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "util.h"

uint32_t *
generate_test_buffer(const struct test *t, size_t tlen, size_t *bufsiz)
{
	size_t i, j, off;
	uint32_t *buf;

	/* allocate and generate buffer */
	for (i = 0, *bufsiz = 0; i < tlen; i++) {
		*bufsiz += t[i].cplen;
	}
	if (!(buf = calloc(*bufsiz, sizeof(*buf)))) {
		fprintf(stderr, "generate_test_buffer: calloc: Out of memory.\n");
		return NULL;
	}
	for (i = 0, off = 0; i < tlen; i++) {
		for (j = 0; j < t[i].cplen; j++) {
			buf[off + j] = t[i].cp[j];
		}
		off += t[i].cplen;
	}

	return buf;
}

static double
time_diff(struct timespec *a, struct timespec *b)
{
	return (double)(b->tv_sec - a->tv_sec) +
	       (double)(b->tv_nsec - a->tv_nsec) * 1E-9;
}

void
run_benchmark(void (*func)(const void *), const void *payload,
              const char *name, const char *unit, double *baseline,
              size_t num_iterations, size_t units_per_iteration)
{
	struct timespec start, end;
	size_t i;
	double diff;

	printf("\t%s ", name);
	fflush(stdout);

	clock_gettime(CLOCK_MONOTONIC, &start);
	for (i = 0; i < num_iterations; i++) {
		func(payload);

		if (i % (num_iterations / 10) == 0) {
			printf(".");
			fflush(stdout);
		}
	}
	clock_gettime(CLOCK_MONOTONIC, &end);
	diff = time_diff(&start, &end) / num_iterations / units_per_iteration;

	if (isnan(*baseline)) {
		*baseline = diff;
		printf(" avg. %.3es/%s (baseline)\n", diff, unit);
	} else {
		printf(" avg. %.3es/%s (%.2f%% %s)\n", diff, unit,
		       fabs(1.0 - diff / *baseline) * 100,
		       (diff < *baseline) ? "faster" : "slower");
	}
}
