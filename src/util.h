/* See LICENSE file for copyright and license details. */
#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>
#include <stdint.h>

#include "../gen/types.h"
#include "../grapheme.h"

#define LEN(x) (sizeof(x) / sizeof(*(x)))

#undef likely
#undef unlikely
#ifdef __has_builtin
	#if __has_builtin(__builtin_expect)
		#define likely(expr) __builtin_expect(!!(expr), 1)
		#define unlikely(expr) __builtin_expect(!!(expr), 0)
	#else
		#define likely(expr) (expr)
		#define unlikely(expr) (expr)
	#endif
#else
	#define likely(expr) (expr)
	#define unlikely(expr) (expr)
#endif

size_t get_codepoint(const void *, size_t, size_t, uint_least32_t *);
size_t get_codepoint_utf8(const void *, size_t, size_t, uint_least32_t *);

size_t set_codepoint(uint_least32_t, void *, size_t, size_t);
size_t set_codepoint_utf8(uint_least32_t, void *, size_t, size_t);

#endif /* UTIL_H */
