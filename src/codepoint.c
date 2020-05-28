/* See LICENSE file for copyright and license details. */
#include "codepoint.h"
#include <stdio.h>

#define BETWEEN(c, l, u) (c >= l && c <= u)
#define LEN(x) (sizeof(x) / sizeof(*x))

/* lookup-table for the types of sequence first bytes */
static const struct {
	uint8_t  lower; /* lower bound of sequence first byte */
	uint8_t  upper; /* upper bound of sequence first byte */
	uint32_t mincp; /* smallest non-overlong encoded codepoint */
	/*
	 * implicit: table-offset represents the number of following
	 * bytes of the form 10xxxxxx (6 bits capacity each)
	 */
} lut[] = {
	[0] = {
		/* 0xxxxxxx */
		.lower = 0x00, /* 00000000 */
		.upper = 0x7F, /* 01111111 */
		.mincp = (uint32_t)0,
	},
	[1] = {
		/* 110xxxxx */
		.lower = 0xC0, /* 11000000 */
		.upper = 0xDF, /* 11011111 */
		.mincp = (uint32_t)1 << 7, /* [0] has 7 bits capacity */
	},
	[2] = {
		/* 1110xxxx */
		.lower = 0xE0, /* 11100000 */
		.upper = 0xEF, /* 11101111 */
		.mincp = (uint32_t)1 << 11, /* [1] has 5+6=11 bits capacity */
	},
	[3] = {
		/* 11110xxx */
		.lower = 0xF0, /* 11110000 */
		.upper = 0xF7, /* 11110111 */
		.mincp = (uint32_t)1 << 16, /* [2] has 4+6+6=16 bits capacity */
	},
};

size_t
grapheme_cp_decode(uint32_t *cp, const uint8_t *s, size_t n)
{
	size_t off, i;

	if (n == 0) {
		/* a sequence must be at least 1 byte long */
		*cp = CP_INVALID;
		return 1;
	}

	/* identify sequence type with the first byte */
	for (off = 0; off < LEN(lut); off++) {
		if (BETWEEN(s[0], lut[off].lower, lut[off].upper)) {
			/*
			 * first byte is within the bounds; fill
			 * p with the the first bits contained in
			 * the first byte ("upper-lower" is the bitmask)
			 */
			*cp = s[0] & (lut[off].upper - lut[off].lower);
			break;
		}
	}
	if (off == LEN(lut)) {
		/*
		 * first byte does not match a sequence type;
		 * set cp as invalid and return 1 byte processed
		 */
		*cp = CP_INVALID;
		return 1;
	}
	if (1 + off > n) {
		/*
		 * input is not long enough, set cp as invalid and
		 * return number of bytes needed
		 */
		*cp = CP_INVALID;
		return 1 + off;
	}

	/*
	 * process 'off' following bytes, each of the form 10xxxxxx
	 * (i.e. between 0x80 (10000000) and 0xBF (10111111))
	 */
	for (i = 1; i <= off; i++) {
		if(!BETWEEN(s[i], 0x80, 0xBF)) {
			/*
			 * byte does not match format; return
			 * number of bytes processed excluding the
			 * unexpected character as recommended since
			 * Unicode 6 (chapter 3)
			 */
			*cp = CP_INVALID;
			return 1 + (i - 1);
		}
		/*
		 * shift codepoint by 6 bits and add the 6 stored bits
		 * in s[i] to it using the bitmask 0x3F (00111111)
		 */
		*cp = (*cp << 6) | (s[i] & 0x3F);
	}

	if (*cp < lut[off].mincp ||
	    BETWEEN(*cp, UINT32_C(0xD800), UINT32_C(0xDFFF)) ||
	    *cp > UINT32_C(0x10FFFF)) {
		/*
		 * codepoint is overlong encoded in the sequence, is a
		 * high or low UTF-16 surrogate half (0xD800..0xDFFF) or
		 * not representable in UTF-16 (>0x10FFFF) (RFC-3629
		 * specifies the latter two conditions)
		 */
		*cp = CP_INVALID;
	}

	return 1 + off;
}
