/* See LICENSE file for copyright and license details. */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../grapheme.h"

#define LEN(x) (sizeof(x) / sizeof(*(x)))

static const struct {
	uint_least32_t cp;      /* input code point */
	uint8_t       *exp_arr; /* expected UTF-8 byte sequence */
	size_t         exp_len; /* expected length of UTF-8 sequence */
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

int
main(void)
{
	size_t i, j, failed;

	/* UTF-8 encoder test */
	for (i = 0, failed = 0; i < LEN(enc_test); i++) {
		uint8_t arr[4];
		size_t len;

		len = lg_utf8_encode(enc_test[i].cp, arr, LEN(arr));

		if (len != enc_test[i].exp_len ||
		    memcmp(arr, enc_test[i].exp_arr, len)) {
			fprintf(stderr, "Failed UTF-8-encoder test %zu: "
			        "Expected (", i);
			for (j = 0; j < enc_test[i].exp_len; j++) {
				fprintf(stderr, "0x%x",
				        enc_test[i].exp_arr[j]);
				if (j + 1 < enc_test[i].exp_len) {
					fprintf(stderr, " ");
				}
			}
			fprintf(stderr, "), but got (");
			for (j = 0; j < len; j++) {
				fprintf(stderr, "0x%x", arr[j]);
				if (j + 1 < len) {
					fprintf(stderr, " ");
				}
			}
			fprintf(stderr, ")\n");
			failed++;
		}
	}
	printf("UTF-8 encoder test: Passed %zu out of %zu tests.\n",
	       LEN(enc_test) - failed, LEN(enc_test));

	return (failed > 0) ? 1 : 0;
}
