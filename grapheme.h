/* See LICENSE file for copyright and license details. */
#ifndef GRAPHEME_H
#define GRAPHEME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct lg_internal_heisenstate {
	uint_least64_t determined;
	uint_least64_t state;
};

typedef struct lg_internal_segmentation_state {
	struct lg_internal_heisenstate a;
	struct lg_internal_heisenstate b;
	uint_least16_t flags;
} LG_SEGMENTATION_STATE;

#define LG_INVALID_CODE_POINT UINT32_C(0xFFFD)

size_t lg_grapheme_nextbreak(const char *);

bool lg_grapheme_isbreak(uint_least32_t, uint_least32_t, LG_SEGMENTATION_STATE *);

size_t lg_utf8_decode(const char *, size_t, uint_least32_t *);
size_t lg_utf8_encode(uint_least32_t, char *, size_t);

#endif /* GRAPHEME_H */
