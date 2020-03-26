/* See LICENSE file for copyright and license details. */
#include "codepoint.h"
#include <stdio.h>

#define BETWEEN(c, l, u) (c >= l && c <= u)
#define LEN(x) (sizeof(x) / sizeof(*x))

size_t
cp_decode(const uint8_t *str, Codepoint *p)
{
	size_t off, j, k, l;

	struct {
		uint8_t lower;
		uint8_t upper;
		uint8_t mask;
		uint8_t bits;
	} lookup[] = {
		{ 0x00, 0x7F, 0xFF,  7 }, /* 00000000 - 01111111, 01111111 */
		{ 0xC0, 0xDF, 0x1F, 11 }, /* 11000000 - 11011111, 00011111 */
		{ 0xE0, 0xEF, 0x0F, 16 }, /* 11100000 - 11101111, 00001111 */
		{ 0xF0, 0xF7, 0x07, 21 }, /* 11110000 - 11110111, 00000111 */
		{ 0xF8, 0xFB, 0x03, 26 }, /* 11111000 - 11111011, 00000011 */
		{ 0xFC, 0xFD, 0x01, 31 }, /* 11111100 - 11111101, 00000001 */
	};

	/* empty string */
	if (str[0] == '\0') {
		*p = 0;
		return 0;
	}

	/* find out in which ranges str[0] is */
	for (off = 0; off < LEN(lookup); off++) {
		if (BETWEEN(str[0], lookup[off].lower, lookup[off].upper)) {
			*p = str[0] & lookup[off].mask;
			break;
		}
	}
	if (off == 0) {
		/* ASCII */
		return 1;
	} else if (off == LEN(lookup)) {
		/* not in ranges */
		*p = CP_INVALID;
		return 1;
	}

	/* off denotes the number of upcoming expected code units */
	for (j = 0; j < off; j++) {
		if (str[j] == '\0') {
			*p = CP_INVALID;
			return j;
		}
		if ((str[1 + j] & 0xC0) != 0x80) {
			*p = CP_INVALID;
			return 1 + j;
		}
		*p <<= 6;
		*p |= str[1 + j] & 0x3F; /* 00111111 */
	}

	if (*p == 0) {
		if (off != 0) {
			/* overencoded NUL */
			*p = CP_INVALID;
		}
	} else {
		/* determine effective bytes */
		for (k = 0; ((*p << k) & (1 << 31)) == 0; k++)
			;
		for (l = 0; l < off; l++) {
			if ((32 - k) <= lookup[l].bits) {
				*p = CP_INVALID;
			}
		}
	}

	return 1 + j;
}
