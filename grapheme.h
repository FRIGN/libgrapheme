/* See LICENSE file for copyright and license details. */
#ifndef GRAPHEME_H
#define GRAPHEME_H

#include <stddef.h>
#include <stdint.h>

#define LG_CODEPOINT_INVALID UINT32_C(0xFFFD)

size_t lg_utf8_decode(uint32_t *, const uint8_t *, size_t);
size_t lg_utf8_encode(uint32_t, uint8_t *, size_t);

size_t lg_grapheme_nextbreak(const char *);
int lg_grapheme_isbreak(uint32_t, uint32_t, int *);

#endif /* GRAPHEME_H */
