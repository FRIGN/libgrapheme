/* See LICENSE file for copyright and license details. */
#include <stddef.h>
#include <stdint.h>

#include "../grapheme.h"
#include "util.h"

#define BETWEEN(c, l, u) ((c) >= (l) && (c) <= (u))

size_t
grapheme_decode_utf8(const char *str, size_t len, uint_least32_t *cp)
{
	size_t i;
	uint_least32_t tmp;
	uint_least32_t mask;

	if (cp == NULL) {
		/*
		 * instead of checking every time if cp is NULL within
		 * the decoder, simply point it at a dummy variable here.
		 */
		cp = &tmp;
	}

	if (str == NULL || len == 0) {
		/* a sequence must be at least 1 byte long */
		*cp = GRAPHEME_INVALID_CODEPOINT;
		return 0;
	}

	*cp = ((const unsigned char *)str)[0];
	if (((const unsigned char *)str)[0] <= 0x7F) {
		/*
		 * ASCII range (0x00..0x7F); we assume ASCII characters
		 * would appear the most often
		 */
		return 1;
	}

	if (!BETWEEN(((const unsigned char *)str)[0], 0xC2, 0xF4)) {
		/*
		 * first byte does not match a sequence type;
		 * set cp as invalid and return 1 byte processed
		 *
		 * this also includes the cases where bits higher than
		 * the 8th are set on systems with CHAR_BIT > 8
		 */
		*cp = GRAPHEME_INVALID_CODEPOINT;
		return 1;
	}

	mask = 1 << 6;
	i = 1;
	do {
		mask <<= 5;

		*cp <<= 6;
		if (i >= len) {
			/*
			 * input is not long enough, continue the loop
			 * to find the number of bytes expected for
			 * this sequence.
			 */
			i++;
			continue;
		}
		if (((const unsigned char *)str)[i] - 0x80U > 0x3FU) {
			/* not a valid continuation byte */
			*cp = GRAPHEME_INVALID_CODEPOINT;
			return i;
		}
		*cp |= ((const unsigned char *)str)[i] - 0x80U;

		/*
		 * RFC-3629 requires us to treat the following cases as
		 * invalid sequences
		 */
		if ((*cp >> 5) == 0x1C0) {
			/*
			 * first byte 0xE0, second byte 0x80..0x9F;
			 * would form a 3-byte overlong sequence
			 */
			*cp = GRAPHEME_INVALID_CODEPOINT;
			return i;
		}
		if ((*cp >> 5) == 0x1DB) {
			/*
			 * first byte 0xED, second byte 0xA0..0xBF;
			 * would fall in the range of UTF-16 surrogates
			 * (0xD800..0xDFFF)
			 */
			*cp = GRAPHEME_INVALID_CODEPOINT;
			return i;
		}
		if ((((*cp ^ 0x100) - 0x3C10) >> 8) == 0) {
			/*
			 * first byte 0xF0, second byte 0x80..0x8F or
			 * first byte 0xF4, second byte 0x90..0xBF;
			 * this would either form a 4-byte overlong
			 * sequence or be not representable in UTF-16
			 * (>0x10FFFF); two conditions checked together
			 * with a bit hack
			 */
			*cp = GRAPHEME_INVALID_CODEPOINT;
			return i;
		}

		i++;
	} while ((*cp & mask) != 0);

	*cp = (i <= len) ?
		(*cp & (mask - 1)) : GRAPHEME_INVALID_CODEPOINT;
	return i;
}

size_t
grapheme_encode_utf8(uint_least32_t cp, char *str, size_t len)
{
	size_t exp_len, i;
	uint_least32_t first_byte;
	unsigned int mask;

	exp_len = 1;
	first_byte = cp;
	if (cp <= 0x7F) {
		/*
		 * ASCII range (0x00..0x7F); we assume ASCII characters
		 * would appear the most often
		 */
		mask = 0x7F;
	} else {
		if (BETWEEN(cp, UINT32_C(0xD800), UINT32_C(0xDFFF)) ||
		    cp > UINT32_C(0x10FFFF)) {
			/*
			 * codepoint is a UTF-16 surrogate
			 * (0xD800..0xDFFF) or not representable in
			 * UTF-16 (>0x10FFFF), which RFC-3629 deems
			 * invalid for UTF-8.
			 */
			cp = GRAPHEME_INVALID_CODEPOINT;
			first_byte = cp;
		}

		/* find the length needed to encode this codepoint */
		mask = 0x3F;
		do {
			exp_len++;
			first_byte >>= 6;
			mask >>= 1;
		} while (first_byte > mask);
	}
	if (exp_len <= len) {
		/* build sequence by filling cp-bits into each byte */
		((unsigned char *)str)[0] =
			(uint_least8_t)((first_byte + (~mask << 1)) & 0xFF);
		for (i = exp_len - 1; i > 0; i--) {
			/*
			 * the bit-format for following bytes is 10000000 (0x80)
			 * and it each stores 6 bits in the 6 low bits that we
			 * extract from the properly-shifted value using the
			 * mask 00111111 (0x3F)
			 */
			((unsigned char *)str)[i] = 0x80 | (cp & 0x3F);
			cp >>= 6;
		}
	}
	return exp_len;
}
