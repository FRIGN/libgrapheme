/* See LICENSE file for copyright and license details. */
#include <stdio.h>

#include "../grapheme.h"
#include "util.h"

#define BETWEEN(c, l, u) (c >= l && c <= u)

/* lookup-table for the types of sequence first bytes */
static const struct {
	uint8_t        lower; /* lower bound of sequence first byte */
	uint8_t        upper; /* upper bound of sequence first byte */
	uint_least32_t mincp; /* smallest non-overlong encoded code point */
	uint_least32_t maxcp; /* largest encodable code point */
	/*
	 * implicit: table-offset represents the number of following
	 * bytes of the form 10xxxxxx (6 bits capacity each)
	 */
} lut[] = {
	[0] = {
		/* 0xxxxxxx */
		.lower = 0x00, /* 00000000 */
		.upper = 0x7F, /* 01111111 */
		.mincp = (uint_least32_t)0,
		.maxcp = ((uint_least32_t)1 << 7) - 1, /* 7 bits capacity */
	},
	[1] = {
		/* 110xxxxx */
		.lower = 0xC0, /* 11000000 */
		.upper = 0xDF, /* 11011111 */
		.mincp = (uint_least32_t)1 << 7,
		.maxcp = ((uint_least32_t)1 << 11) - 1, /* 5+6=11 bits capacity */
	},
	[2] = {
		/* 1110xxxx */
		.lower = 0xE0, /* 11100000 */
		.upper = 0xEF, /* 11101111 */
		.mincp = (uint_least32_t)1 << 11,
		.maxcp = ((uint_least32_t)1 << 16) - 1, /* 4+6+6=16 bits capacity */
	},
	[3] = {
		/* 11110xxx */
		.lower = 0xF0, /* 11110000 */
		.upper = 0xF7, /* 11110111 */
		.mincp = (uint_least32_t)1 << 16,
		.maxcp = ((uint_least32_t)1 << 21) - 1, /* 3+6+6+6=21 bits capacity */
	},
};

size_t
lg_utf8_decode(const char *s, size_t n, uint_least32_t *cp)
{
	size_t off, i;

	if (s == NULL || n == 0) {
		/* a sequence must be at least 1 byte long */
		*cp = LG_INVALID_CODE_POINT;
		return 0;
	}

	/* identify sequence type with the first byte */
	for (off = 0; off < LEN(lut); off++) {
		if (BETWEEN(((const unsigned char *)s)[0], lut[off].lower,
		            lut[off].upper)) {
			/*
			 * first byte is within the bounds; fill
			 * p with the the first bits contained in
			 * the first byte (by subtracting the high bits)
			 */
			*cp = ((const unsigned char *)s)[0] - lut[off].lower;
			break;
		}
	}
	if (off == LEN(lut)) {
		/*
		 * first byte does not match a sequence type;
		 * set cp as invalid and return 1 byte processed
		 *
		 * this also includes the cases where bits higher than
		 * the 8th are set on systems with CHAR_BIT > 8
		 */
		*cp = LG_INVALID_CODE_POINT;
		return 1;
	}
	if (1 + off > n) {
		/*
		 * input is not long enough, set cp as invalid and
		 * return number of bytes needed
		 */
		*cp = LG_INVALID_CODE_POINT;
		return 1 + off;
	}

	/*
	 * process 'off' following bytes, each of the form 10xxxxxx
	 * (i.e. between 0x80 (10000000) and 0xBF (10111111))
	 */
	for (i = 1; i <= off; i++) {
		if(!BETWEEN(((const unsigned char *)s)[i], 0x80, 0xBF)) {
			/*
			 * byte does not match format; return
			 * number of bytes processed excluding the
			 * unexpected character as recommended since
			 * Unicode 6 (chapter 3)
			 *
			 * this also includes the cases where bits
			 * higher than the 8th are set on systems
			 * with CHAR_BIT > 8
			 */
			*cp = LG_INVALID_CODE_POINT;
			return 1 + (i - 1);
		}
		/*
		 * shift code point by 6 bits and add the 6 stored bits
		 * in s[i] to it using the bitmask 0x3F (00111111)
		 */
		*cp = (*cp << 6) | (((const unsigned char *)s)[i] & 0x3F);
	}

	if (*cp < lut[off].mincp ||
	    BETWEEN(*cp, UINT32_C(0xD800), UINT32_C(0xDFFF)) ||
	    *cp > UINT32_C(0x10FFFF)) {
		/*
		 * code point is overlong encoded in the sequence, is a
		 * high or low UTF-16 surrogate half (0xD800..0xDFFF) or
		 * not representable in UTF-16 (>0x10FFFF) (RFC-3629
		 * specifies the latter two conditions)
		 */
		*cp = LG_INVALID_CODE_POINT;
	}

	return 1 + off;
}

size_t
lg_utf8_encode(uint_least32_t cp, char *s, size_t n)
{
	size_t off, i;

	if (BETWEEN(cp, UINT32_C(0xD800), UINT32_C(0xDFFF)) ||
	    cp > UINT32_C(0x10FFFF)) {
		/*
		 * code point is a high or low UTF-16 surrogate half
		 * (0xD800..0xDFFF) or not representable in UTF-16
		 * (>0x10FFFF), which RFC-3629 deems invalid for UTF-8.
		 */
		cp = LG_INVALID_CODE_POINT;
	}

	/* determine necessary sequence type */
	for (off = 0; off < LEN(lut); off++) {
		if (cp <= lut[off].maxcp) {
			break;
		}
	}
	if (1 + off > n || s == NULL || n == 0) {
		/*
		 * specified buffer is too small to store sequence or
		 * the caller just wanted to know how many bytes the
		 * codepoint needs by passing a NULL-buffer.
		 */
		return 1 + off;
	}

	/* build sequence by filling cp-bits into each byte */

	/*
	 * lut[off].lower is the bit-format for the first byte and
	 * the bits to fill into it are determined by shifting the
	 * cp 6 times the number of following bytes, as each
	 * following byte stores 6 bits, yielding the wanted bits.
	 *
	 * We do not overwrite the mask because we guaranteed earlier
	 * that there are no bits higher than the mask allows.
	 */
	((unsigned char *)s)[0] = lut[off].lower | (uint8_t)(cp >> (6 * off));

	for (i = 1; i <= off; i++) {
		/*
		 * the bit-format for following bytes is 10000000 (0x80)
		 * and it each stores 6 bits in the 6 low bits that we
		 * extract from the properly-shifted value using the
		 * mask 00111111 (0x3F)
		 */
		((unsigned char *)s)[i] = 0x80 |
		                          ((cp >> (6 * (off - i))) & 0x3F);
	}

	return 1 + off;
}
