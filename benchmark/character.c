/* See LICENSE file for copyright and license details. */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../grapheme.h"
#include "../gen/character-test.h"
#include "util.h"

#include <unigbrk.h>
#include <utf8proc.h>

#define NUM_ITERATIONS 10000

#if defined __has_attribute
	#if __has_attribute(optnone)
		void libgrapheme(const uint32_t *, size_t) __attribute__((optnone));
		void libutf8proc(const uint32_t *, size_t) __attribute__((optnone));
	#endif
#endif

void
libgrapheme(const uint32_t *buf, size_t bufsiz)
{
	GRAPHEME_STATE state = { 0 };
	size_t i;

	for (i = 0; i + 1 < bufsiz; i++) {
		(void)grapheme_is_character_break(buf[i], buf[i+1], &state);
	}
}

void
libutf8proc(const uint32_t *buf, size_t bufsiz)
{
	utf8proc_int32_t state = 0;
	size_t i;

	for (i = 0; i + 1 < bufsiz; i++) {
		(void)utf8proc_grapheme_break_stateful(buf[i], buf[i+1], &state);
	}
}

int
main(int argc, char *argv[])
{
	size_t bufsiz;
	uint32_t *buf;
	double baseline = NAN;

	(void)argc;

	if ((buf = generate_test_buffer(character_test, LEN(character_test),
	                                &bufsiz)) == NULL) {
		return 1;
	}

	printf("%s\n", argv[0]);
	run_benchmark(libgrapheme, "libgrapheme ", &baseline, buf, bufsiz,
	              NUM_ITERATIONS);
	run_benchmark(libutf8proc, "libutf8proc ", &baseline, buf, bufsiz,
	              NUM_ITERATIONS);

	free(buf);

	return 0;
}
