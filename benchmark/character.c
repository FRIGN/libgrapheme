/* See LICENSE file for copyright and license details. */
#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../grapheme.h"
#include "../gen/character-test.h"
#include "util.h"

#include <utf8proc.h>

#define NUM_ITERATIONS 100000

#ifdef __has_attribute
	#if __has_attribute(optnone)
		void libgrapheme(const void *) __attribute__((optnone));
		void libutf8proc(const void *) __attribute__((optnone));
	#endif
#endif

struct payload {
	uint_least32_t *buf;
	utf8proc_int32_t *buf_int32;
	size_t bufsiz;
};

void
libgrapheme(const void *payload)
{
	GRAPHEME_STATE state = { 0 };
	const struct payload *p = payload;
	size_t i;

	for (i = 0; i + 1 < p->bufsiz; i++) {
		(void)grapheme_is_character_break(p->buf[i], p->buf[i+1],
		                                  &state);
	}
}

void
libutf8proc(const void *payload)
{
	utf8proc_int32_t state = 0;
	const struct payload *p = payload;
	size_t i;

	for (i = 0; i + 1 < p->bufsiz; i++) {
		(void)utf8proc_grapheme_break_stateful(p->buf_int32[i],
		                                       p->buf_int32[i+1],
		                                       &state);
	}
}

int
main(int argc, char *argv[])
{
	struct payload p;
	double baseline = (double)NAN;
	size_t i;

	(void)argc;

	if ((p.buf = generate_test_buffer(character_test, LEN(character_test),
	                                  &(p.bufsiz))) == NULL) {
		return 1;
	}
	if ((p.buf_int32 = malloc(p.bufsiz * sizeof(*(p.buf_int32)))) == NULL) {
		fprintf(stderr, "malloc: %s\n", strerror(errno));
		exit(1);
	}
	for (i = 0; i < p.bufsiz; i++) {
		/*
		 * there is no overflow, as we know that the maximum
		 * codepoint is 0x10FFFF, which is way below 2^31
		 */
		p.buf_int32[i] = (utf8proc_int32_t)p.buf[i];
	}

	printf("%s\n", argv[0]);
	run_benchmark(libgrapheme, &p, "libgrapheme ", &baseline,
	              NUM_ITERATIONS);
	run_benchmark(libutf8proc, &p, "libutf8proc ", &baseline,
	              NUM_ITERATIONS);

	free(p.buf);

	return 0;
}
