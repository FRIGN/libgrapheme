/* See LICENSE file for copyright and license details. */
#ifndef GRAPHEME_H
#define GRAPHEME_H

#include <stddef.h>
#include <stdint.h>

#define GRAPHEME_CP_INVALID UINT32_C(0xFFFD)

int grapheme_boundary(uint32_t, uint32_t, int *);

size_t grapheme_cp_decode(uint32_t *, const uint8_t *, size_t);
size_t grapheme_cp_encode(uint32_t, uint8_t *, size_t);

size_t grapheme_len(const char *);

#endif /* GRAPHEME_H */
