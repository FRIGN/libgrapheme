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

#endif /* UTIL_H */
