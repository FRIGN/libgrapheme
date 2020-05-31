/* See LICENSE file for copyright and license details. */
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "boundary.h"
#include "codepoint.h"

#define LEN(x) (sizeof(x) / sizeof(*x))

static const struct {
	uint32_t cp;      /* input code point */
	uint8_t *exp_arr; /* expected UTF-8 byte sequence */
	size_t   exp_len; /* expected length of UTF-8 sequence */
} enc_test[] = {
	{
		/* invalid code point (UTF-16 surrogate half) */
		.cp      = UINT32_C(0xD800),
		.exp_arr = (uint8_t[]){ 0xEF, 0xBF, 0xBD },
		.exp_len = 3,
	},
	{
		/* invalid code point (UTF-16-unrepresentable) */
		.cp      = UINT32_C(0x110000),
		.exp_arr = (uint8_t[]){ 0xEF, 0xBF, 0xBD },
		.exp_len = 3,
	},
	{
		/* code point encoded to a 1-byte sequence */
		.cp      = 0x01,
		.exp_arr = (uint8_t[]){ 0x01 },
		.exp_len = 1,
	},
	{
		/* code point encoded to a 2-byte sequence */
		.cp      = 0xFF,
		.exp_arr = (uint8_t[]){ 0xC3, 0xBF },
		.exp_len = 2,
	},
	{
		/* code point encoded to a 3-byte sequence */
		.cp      = 0xFFF,
		.exp_arr = (uint8_t[]){ 0xE0, 0xBF, 0xBF },
		.exp_len = 3,
	},
	{
		/* code point encoded to a 4-byte sequence */
		.cp      = UINT32_C(0xFFFFF),
		.exp_arr = (uint8_t[]){ 0xF3, 0xBF, 0xBF, 0xBF },
		.exp_len = 4,
	},
};

static const struct {
	uint8_t *arr;     /* UTF-8 byte sequence */
	size_t   len;     /* length of UTF-8 byte sequence */
	size_t   exp_len; /* expected length returned */
	uint32_t exp_cp;  /* expected code point returned */
} dec_test[] = {
	{
		/* empty sequence
		 * [ ] ->
		 * INVALID
		 */
		.arr     = NULL,
		.len     = 0,
		.exp_len = 1,
		.exp_cp  = CP_INVALID,
	},
	{
		/* invalid lead byte
		 * [ 11111101 ] ->
		 * INVALID
		 */
		.arr     = (uint8_t[]){ 0xFD },
		.len     = 1,
		.exp_len = 1,
		.exp_cp  = CP_INVALID,
	},
	{
		/* valid 1-byte sequence
		 * [ 00000001 ] ->
		 * 0000001
		 */
		.arr     = (uint8_t[]){ 0x01 },
		.len     = 1,
		.exp_len = 1,
		.exp_cp  = 0x1,
	},
	{
		/* valid 2-byte sequence
		 * [ 11000011 10111111 ] ->
		 * 00011111111
		 */
		.arr     = (uint8_t[]){ 0xC3, 0xBF },
		.len     = 2,
		.exp_len = 2,
		.exp_cp  = 0xff,
	},
	{
		/* invalid 2-byte sequence (second byte missing)
		 * [ 11000011 ] ->
		 * INVALID
		 */
		.arr     = (uint8_t[]){ 0xC3 },
		.len     = 1,
		.exp_len = 2,
		.exp_cp  = CP_INVALID,
	},
	{
		/* invalid 2-byte sequence (second byte malformed)
		 * [ 11000011 11111111 ] ->
		 * INVALID
		 */
		.arr     = (uint8_t[]){ 0xC3, 0xFF },
		.len     = 2,
		.exp_len = 1,
		.exp_cp  = CP_INVALID,
	},
	{
		/* invalid 2-byte sequence (overlong encoded)
		 * [ 11000001 10111111 ] ->
		 * INVALID
		 */
		.arr     = (uint8_t[]){ 0xC1, 0xBF },
		.len     = 2,
		.exp_len = 2,
		.exp_cp  = CP_INVALID,
	},
	{
		/* valid 3-byte sequence
		 * [ 11100000 10111111 10111111 ] ->
		 * 0000111111111111
		 */
		.arr     = (uint8_t[]){ 0xE0, 0xBF, 0xBF },
		.len     = 3,
		.exp_len = 3,
		.exp_cp  = 0xfff,
	},
	{
		/* invalid 3-byte sequence (second byte missing)
		 * [ 11100000 ] ->
		 * INVALID
		 */
		.arr     = (uint8_t[]){ 0xE0 },
		.len     = 1,
		.exp_len = 3,
		.exp_cp  = CP_INVALID,
	},
	{
		/* invalid 3-byte sequence (second byte malformed)
		 * [ 11100000 01111111 10111111 ] ->
		 * INVALID
		 */
		.arr     = (uint8_t[]){ 0xE0, 0x7F, 0xBF },
		.len     = 3,
		.exp_len = 1,
		.exp_cp  = CP_INVALID,
	},
	{
		/* invalid 3-byte sequence (third byte missing)
		 * [ 11100000 10111111 ] ->
		 * INVALID
		 */
		.arr     = (uint8_t[]){ 0xE0, 0xBF },
		.len     = 2,
		.exp_len = 3,
		.exp_cp  = CP_INVALID,
	},
	{
		/* invalid 3-byte sequence (third byte malformed)
		 * [ 11100000 10111111 01111111 ] ->
		 * INVALID
		 */
		.arr     = (uint8_t[]){ 0xE0, 0xBF, 0x7F },
		.len     = 3,
		.exp_len = 2,
		.exp_cp  = CP_INVALID,
	},
	{
		/* invalid 3-byte sequence (overlong encoded)
		 * [ 11100000 10011111 10111111 ] ->
		 * INVALID
		 */
		.arr     = (uint8_t[]){ 0xE0, 0x9F, 0xBF },
		.len     = 3,
		.exp_len = 3,
		.exp_cp  = CP_INVALID,
	},
	{
		/* invalid 3-byte sequence (UTF-16 surrogate half)
		 * [ 11101101 10100000 10000000 ] ->
		 * INVALID
		 */
		.arr     = (uint8_t[]){ 0xED, 0xA0, 0x80 },
		.len     = 3,
		.exp_len = 3,
		.exp_cp  = CP_INVALID,
	},
	{
		/* valid 4-byte sequence
		 * [ 11110011 10111111 10111111 10111111 ] ->
		 * 011111111111111111111
		 */
		.arr     = (uint8_t[]){ 0xF3, 0xBF, 0xBF, 0xBF },
		.len     = 4,
		.exp_len = 4,
		.exp_cp  = 0xfffff,
	},
	{
		/* invalid 4-byte sequence (second byte missing)
		 * [ 11110011 ] ->
		 * INVALID
		 */
		.arr     = (uint8_t[]){ 0xF3 },
		.len     = 1,
		.exp_len = 4,
		.exp_cp  = CP_INVALID,
	},
	{
		/* invalid 4-byte sequence (second byte malformed)
		 * [ 11110011 01111111 10111111 10111111 ] ->
		 * INVALID
		 */
		.arr     = (uint8_t[]){ 0xF3, 0x7F, 0xBF, 0xBF },
		.len     = 4,
		.exp_len = 1,
		.exp_cp  = CP_INVALID,
	},
	{
		/* invalid 4-byte sequence (third byte missing)
		 * [ 11110011 10111111 ] ->
		 * INVALID
		 */
		.arr     = (uint8_t[]){ 0xF3, 0xBF },
		.len     = 2,
		.exp_len = 4,
		.exp_cp  = CP_INVALID,
	},
	{
		/* invalid 4-byte sequence (third byte malformed)
		 * [ 11110011 10111111 01111111 10111111 ] ->
		 * INVALID
		 */
		.arr     = (uint8_t[]){ 0xF3, 0xBF, 0x7F, 0xBF },
		.len     = 4,
		.exp_len = 2,
		.exp_cp  = CP_INVALID,
	},
	{
		/* invalid 4-byte sequence (fourth byte missing)
		 * [ 11110011 10111111 10111111 ] ->
		 * INVALID
		 */
		.arr     = (uint8_t[]){ 0xF3, 0xBF, 0xBF },
		.len     = 3,
		.exp_len = 4,
		.exp_cp  = CP_INVALID,
	},
	{
		/* invalid 4-byte sequence (fourth byte malformed)
		 * [ 11110011 10111111 10111111 01111111 ] ->
		 * INVALID
		 */
		.arr     = (uint8_t[]){ 0xF3, 0xBF, 0xBF, 0x7F },
		.len     = 4,
		.exp_len = 3,
		.exp_cp  = CP_INVALID,
	},
	{
		/* invalid 4-byte sequence (overlong encoded)
		 * [ 11110000 10000000 10000001 10111111 ] ->
		 * INVALID
		 */
		.arr     = (uint8_t[]){ 0xF0, 0x80, 0x81, 0xBF },
		.len     = 4,
		.exp_len = 4,
		.exp_cp  = CP_INVALID,
	},
	{
		/* invalid 4-byte sequence (UTF-16-unrepresentable)
		 * [ 11110100 10010000 10000000 10000000 ] ->
		 * INVALID
		 */
		.arr     = (uint8_t[]){ 0xF4, 0x90, 0x80, 0x80 },
		.len     = 4,
		.exp_len = 4,
		.exp_cp  = CP_INVALID,
	},
};

int main(void)
{
	int state;
	size_t i, j, k, len, failed;

	/* UTF-8 encoder test */
	for (i = 0, failed = 0; i < LEN(enc_test); i++) {
		uint8_t arr[4];
		size_t len;

		len = grapheme_cp_encode(enc_test[i].cp, arr, LEN(arr));

		if (len != enc_test[i].exp_len ||
		    memcmp(arr, enc_test[i].exp_arr, len)) {
			fprintf(stderr, "Failed UTF-8-encoder test %zu: "
			        "Expected (", i);
			for (j = 0; j < enc_test[i].exp_len; j++) {
				fprintf(stderr, "0x%x",
				        enc_test[i].exp_arr[j]);
				if (j != enc_test[i].exp_len - 1) {
					fprintf(stderr, " ");
				}
			}
			fprintf(stderr, "), but got (");
			for (j = 0; j < len; j++) {
				fprintf(stderr, "0x%x", arr[j]);
				if (j != len - 1) {
					fprintf(stderr, " ");
				}
			}
			fprintf(stderr, ")\n");
			failed++;
		}
	}
	printf("UTF-8 encoder test: Passed %zu out of %zu tests.\n",
	       LEN(enc_test) - failed, LEN(enc_test));

	/* UTF-8 decoder test */
	for (i = 0, failed = 0; i < LEN(dec_test); i++) {
		size_t len;
		uint32_t cp;

		len = grapheme_cp_decode(&cp, dec_test[i].arr,
		                         dec_test[i].len);

		if (len != dec_test[i].exp_len ||
		    cp != dec_test[i].exp_cp) {
			fprintf(stderr, "Failed UTF-8-decoder test %zu: "
			        "Expected (%zx,%u), but got (%zx,%u)\n",
			        i, dec_test[i].exp_len,
			        dec_test[i].exp_cp, len, cp);
			failed++;
		}
	}
	printf("UTF-8 decoder test: Passed %zu out of %zu tests.\n",
	       LEN(dec_test) - failed, LEN(dec_test));

	/* grapheme break test */
	for (i = 0, failed = 0; i < LEN(t); i++) {
		for (j = 0, k = 0, state = 0, len = 1; j < t[i].cplen; j++) {
			if ((j + 1) == t[i].cplen ||
			    boundary(t[i].cp[j], t[i].cp[j + 1], &state)) {
				/* check if our resulting length matches */
				if (k == t[i].lenlen || len != t[i].len[k++]) {
					fprintf(stderr, "Failed \"%s\"\n",
					        t[i].descr);
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
	       LEN(t) - failed, LEN(t));

	return (failed > 0) ? 1 : 0;
}
