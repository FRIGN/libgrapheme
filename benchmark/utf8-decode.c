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
	char *buf_char;
	utf8proc_uint8_t *buf_uint8;
	size_t bufsiz;
};

void
libgrapheme(const void *payload)
{
	const struct payload *p = payload;
	uint_least32_t cp;
	size_t ret, off;

	for (off = 0; off < p->bufsiz; off += ret) {
		if ((ret = grapheme_decode_utf8(p->buf_char + off,
		                                p->bufsiz - off, &cp)) >
		    (p->bufsiz - off)) {
			break;
		}
		(void)cp;
	}
}

void
libutf8proc(const void *payload)
{
	const struct payload *p = payload;
	utf8proc_int32_t cp;
	utf8proc_ssize_t ret;
	size_t off;

	for (off = 0; off < p->bufsiz; off += (size_t)ret) {
		if ((ret = utf8proc_iterate(p->buf_uint8 + off,
		                            (utf8proc_ssize_t)(p->bufsiz - off),
				            &cp)) < 0) {
			break;
		}
		(void)cp;
	}
}

int
main(int argc, char *argv[])
{
	struct payload p;
	size_t cpbufsiz, i, off, ret;
	uint32_t *cpbuf;
	double baseline = (double)NAN;

	(void)argc;

	if ((cpbuf = generate_test_buffer(character_test, LEN(character_test),
	                                  &cpbufsiz)) == NULL) {
		return 1;
	}

	/* convert cp-buffer to utf8-data (both as char and custom uint8-type) */
	for (i = 0, p.bufsiz = 0; i < cpbufsiz; i++) {
		p.bufsiz += grapheme_encode_utf8(cpbuf[i], NULL, 0);
	}
	if ((p.buf_char = malloc(p.bufsiz)) == NULL) {
		fprintf(stderr, "malloc: %s\n", strerror(errno));
		exit(1);
	}
	for (i = 0, off = 0; i < cpbufsiz; i++, off += ret) {
		if ((ret = grapheme_encode_utf8(cpbuf[i], p.buf_char + off,
		                                p.bufsiz - off)) >
		    (p.bufsiz - off)) {
			/* shouldn't happen */
			fprintf(stderr, "Error while converting buffer.\n");
			exit(1);
		}
	}
	if ((p.buf_uint8 = malloc(p.bufsiz)) == NULL) {	
		fprintf(stderr, "malloc: %s\n", strerror(errno));
		exit(1);
	}
	for (i = 0; i < p.bufsiz; i++) {
		/* 
		 * even if char is larger than 8 bit, it will only have
		 * any of the first 8 bits set (by construction).
		 */
		p.buf_uint8[i] = (utf8proc_uint8_t)p.buf_char[i];
	}

	printf("%s\n", argv[0]);
	run_benchmark(libgrapheme, &p, "libgrapheme ", "byte", &baseline,
	              NUM_ITERATIONS, p.bufsiz);
	run_benchmark(libutf8proc, &p, "libutf8proc ", "byte", &baseline,
	              NUM_ITERATIONS, p.bufsiz);

	free(cpbuf);
	free(p.buf_char);
	free(p.buf_uint8);

	return 0;
}
