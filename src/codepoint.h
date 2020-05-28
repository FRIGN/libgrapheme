/* See LICENSE file for copyright and license details. */
#ifndef CODEPOINT_H
#define CODEPOINT_H

#include <stddef.h>
#include <stdint.h>

typedef uint32_t Codepoint;

#define CP_INVALID 0xFFFD

size_t grapheme_cp_decode(uint32_t *, const uint8_t *, size_t);

#endif /* CODEPOINT_H */
