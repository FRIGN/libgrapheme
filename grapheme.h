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
	struct grapheme_internal_heisenstate a;
	struct grapheme_internal_heisenstate b;
	uint_least16_t flags;
} GRAPHEME_SEGMENTATION_STATE;

#define GRAPHEME_INVALID_CODE_POINT UINT32_C(0xFFFD)

size_t grapheme_character_nextbreak(const char *);

bool grapheme_character_isbreak(uint_least32_t, uint_least32_t,
                                GRAPHEME_SEGMENTATION_STATE *);

size_t grapheme_utf8_decode(const char *, size_t, uint_least32_t *);
size_t grapheme_utf8_encode(uint_least32_t, char *, size_t);

#endif /* GRAPHEME_H */
