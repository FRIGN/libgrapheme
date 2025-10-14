/* See LICENSE file for copyright and license details. */
#ifndef CHARACTER_H
#define CHARACTER_H

#include <stdint.h>

#define CHAR_PROP_CONTROL               (UINT64_C(1) <<  0)
#define CHAR_PROP_EXTEND                (UINT64_C(1) <<  1)
#define CHAR_PROP_EXTENDED_PICTOGRAPHIC (UINT64_C(1) <<  2)
#define CHAR_PROP_HANGUL_L              (UINT64_C(1) <<  3)
#define CHAR_PROP_HANGUL_V              (UINT64_C(1) <<  4)
#define CHAR_PROP_HANGUL_T              (UINT64_C(1) <<  5)
#define CHAR_PROP_HANGUL_LV             (UINT64_C(1) <<  6)
#define CHAR_PROP_HANGUL_LVT            (UINT64_C(1) <<  7)
#define CHAR_PROP_ICB_CONSONANT         (UINT64_C(1) <<  8)
#define CHAR_PROP_ICB_EXTEND            (UINT64_C(1) <<  9)
#define CHAR_PROP_ICB_LINKER            (UINT64_C(1) << 10)
#define CHAR_PROP_PREPEND               (UINT64_C(1) << 11)
#define CHAR_PROP_REGIONAL_INDICATOR    (UINT64_C(1) << 12)
#define CHAR_PROP_SPACINGMARK           (UINT64_C(1) << 13)

#endif /* UTIL_H */
