/* See LICENSE file for copyright and license details. */
#ifndef CODEPOINT_H
#define CODEPOINT_H

#include <stddef.h>
#include <stdint.h>

typedef uint32_t Codepoint;

#define CP_INVALID 0xFFFD

size_t cp_decode(const uint8_t *, Codepoint *);

#endif /* CODEPOINT_H */
