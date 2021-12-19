/* See LICENSE file for copyright and license details. */
#ifndef GRAPHEME_H
#define GRAPHEME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct grapheme_internal_heisenstate {
	uint_least64_t determined;
	uint_least64_t state;
};

typedef struct grapheme_internal_segmentation_state {
	struct grapheme_internal_heisenstate cp0;
	struct grapheme_internal_heisenstate cp1;
	uint_least16_t flags;
} GRAPHEME_STATE;

#define GRAPHEME_INVALID_CODEPOINT UINT32_C(0xFFFD)

size_t grapheme_next_character_break(const char *, size_t);

bool grapheme_is_character_break(uint_least32_t, uint_least32_t, GRAPHEME_STATE *);

size_t grapheme_decode_utf8(const char *, size_t, uint_least32_t *);
size_t grapheme_encode_utf8(uint_least32_t, char *, size_t);

#endif /* GRAPHEME_H */
