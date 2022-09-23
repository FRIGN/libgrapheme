/* See LICENSE file for copyright and license details. */
#include <stdbool.h>
#include <stddef.h>

#include "../gen/word.h"
#include "../grapheme.h"
#include "util.h"

static inline enum word_break_property
get_break_prop(uint_least32_t cp)
{
	if (likely(cp <= 0x10FFFF)) {
		return (enum word_break_property)
		       word_break_minor[word_break_major[cp >> 8] + (cp & 0xff)];
	} else {
		return WORD_BREAK_PROP_OTHER;
	}
}

static size_t
next_word_break(const void *str, size_t len, size_t (*get_codepoint)
                (const void *, size_t, size_t, uint_least32_t *))
{
	struct {
		enum word_break_property a, b, c, d;
	} raw, skip;
	enum word_break_property res;
	uint_least32_t cp;
	size_t off, tmp, new_off;
	bool ri_even = true;

	/* check degenerate cases */
	if (str == NULL || len == 0) {
		return 0;
	}

	/*
	 * Apply word breaking algorithm (UAX #29), see
	 * https://unicode.org/reports/tr29/#Word_Boundary_Rules
	 *
	 * There are 4 slots (a, b, c, d) of "break" properties and
	 * we check if there is a break in the middle between b and c.
	 *
	 * The position of this middle spot is determined by off,
	 * which gives the offset of the first element on the right
	 * hand side of said spot, or, in other words, gives the number
	 * of elements on the left hand side.
	 *
	 * It is further complicated by the fact that the algorithm
	 * expects you to skip certain characters for the second
	 * half of the rules (after WB4). Thus, we do not only have
	 * the "raw" properties as described above, but also the "skip"
	 * properties, where the skip.a and skip.b, for instance,
	 * give the two preceding character properties behind the
	 * currently investigated breakpoint.
	 *
	 */

	/*
	 * Initialize the different properties such that we have
	 * a good state after the state-update in the loop
	 */
	raw.b = NUM_WORD_BREAK_PROPS;
	if ((off = get_codepoint(str, len, 0, &cp)) >= len) {
		/*
		 * A line is at least one codepoint long, so we can
		 * safely return here
		 */
		return len;
	}
	raw.c = get_break_prop(cp);
	(void)get_codepoint(str, len, off, &cp);
	raw.d = get_break_prop(cp);
	skip.a = skip.b = NUM_WORD_BREAK_PROPS;

	for (; off < len; off = new_off) {
		/*
		 * Update left side (a and b) of the skip state by
		 * "shifting in" the raw.c property as long as it is
		 * not one of the "ignored" character properties.
		 * While at it, update the RI-counter.
		 *
		 */
		if (raw.c != WORD_BREAK_PROP_EXTEND &&
		    raw.c != WORD_BREAK_PROP_FORMAT &&
		    raw.c != WORD_BREAK_PROP_ZWJ) {
		    	skip.a = skip.b;
			skip.b = raw.c;

			if (skip.b == WORD_BREAK_PROP_REGIONAL_INDICATOR) {
				/*
				 * The property we just shifted in is
				 * a regional indicator, increasing the
				 * number of consecutive RIs on the left
				 * side of the breakpoint by one, changing
				 * the oddness.
				 *
				 */
				ri_even = !ri_even;
			} else {
				/*
				 * We saw no regional indicator, so the
				 * number of consecutive RIs on the left
				 * side of the breakpoint is zero, which
				 * is an even number.
				 *
				 */
				ri_even = true;
			}
		}

		/*
		 * Update right side (b and c) of the skip state by
		 * starting at the breakpoint and detecting the two
		 * following non-ignored character classes
		 *
		 */
		skip.c = NUM_WORD_BREAK_PROPS;
		for (tmp = off; tmp < len; ) {
			tmp += get_codepoint(str, len, tmp, &cp);
			res = get_break_prop(cp);

			if (res != WORD_BREAK_PROP_EXTEND &&
			    res != WORD_BREAK_PROP_FORMAT &&
			    res != WORD_BREAK_PROP_ZWJ) {
				skip.c = res;
				break;
			}
		}
		skip.d = NUM_WORD_BREAK_PROPS;
		for (; tmp < len; ) {
			tmp += get_codepoint(str, len, tmp, &cp);
			res = get_break_prop(cp);

			if (res != WORD_BREAK_PROP_EXTEND &&
			    res != WORD_BREAK_PROP_FORMAT &&
			    res != WORD_BREAK_PROP_ZWJ) {
				skip.d = res;
				break;
			}
		}

		/*
		 * Update the raw state by simply shifting everything
		 * in and, if we still have data left, determining
		 * the character class of the next codepoint.
		 *
		 */
		raw.a = raw.b;
		raw.b = raw.c;
		raw.c = raw.d;
		if ((new_off = off + get_codepoint(str, len, off, &cp)) < len) {
			get_codepoint(str, len, new_off, &cp);
			raw.d = get_break_prop(cp);
		} else {
			raw.d = NUM_WORD_BREAK_PROPS;
		}

		/* WB3 */
		if (raw.b == WORD_BREAK_PROP_CR &&
		    raw.c == WORD_BREAK_PROP_LF) {
			continue;
		}

		/* WB3a */
		if (raw.b == WORD_BREAK_PROP_NEWLINE ||
		    raw.b == WORD_BREAK_PROP_CR      ||
		    raw.b == WORD_BREAK_PROP_LF) {
			break;
		}

		/* WB3b */
		if (raw.c == WORD_BREAK_PROP_NEWLINE ||
		    raw.c == WORD_BREAK_PROP_CR      ||
		    raw.c == WORD_BREAK_PROP_LF) {
			break;
		}

		/* WB3c */
		if (raw.b == WORD_BREAK_PROP_ZWJ &&
		    (raw.c == WORD_BREAK_PROP_EXTENDED_PICTOGRAPHIC ||
		     raw.c == WORD_BREAK_PROP_BOTH_ALETTER_EXTPICT)) {
			continue;
		}

		/* WB3d */
		if (raw.b == WORD_BREAK_PROP_WSEGSPACE &&
		    raw.c == WORD_BREAK_PROP_WSEGSPACE) {
			continue;
		}

		/* WB4 */
		if (raw.c == WORD_BREAK_PROP_EXTEND ||
		    raw.c == WORD_BREAK_PROP_FORMAT ||
		    raw.c == WORD_BREAK_PROP_ZWJ) {
			continue;
		}

		/* WB5 */
		if ((skip.b == WORD_BREAK_PROP_ALETTER              ||
		     skip.b == WORD_BREAK_PROP_BOTH_ALETTER_EXTPICT ||
		     skip.b == WORD_BREAK_PROP_HEBREW_LETTER) &&
		    (skip.c == WORD_BREAK_PROP_ALETTER              ||
		     skip.c == WORD_BREAK_PROP_BOTH_ALETTER_EXTPICT ||
		     skip.c == WORD_BREAK_PROP_HEBREW_LETTER)) {
			continue;
		}

		/* WB6 */
		if ((skip.b == WORD_BREAK_PROP_ALETTER              ||
		     skip.b == WORD_BREAK_PROP_BOTH_ALETTER_EXTPICT ||
		     skip.b == WORD_BREAK_PROP_HEBREW_LETTER) &&
		    (skip.c == WORD_BREAK_PROP_MIDLETTER    ||
		     skip.c == WORD_BREAK_PROP_MIDNUMLET    ||
		     skip.c == WORD_BREAK_PROP_SINGLE_QUOTE) &&
		    len > 2 &&
		    (skip.d == WORD_BREAK_PROP_ALETTER              ||
		     skip.d == WORD_BREAK_PROP_BOTH_ALETTER_EXTPICT ||
		     skip.d == WORD_BREAK_PROP_HEBREW_LETTER)) {
			continue;
		}

		/* WB7 */
		if ((skip.b == WORD_BREAK_PROP_MIDLETTER    ||
		     skip.b == WORD_BREAK_PROP_MIDNUMLET    ||
		     skip.b == WORD_BREAK_PROP_SINGLE_QUOTE) &&
		    (skip.c == WORD_BREAK_PROP_ALETTER              ||
		     skip.c == WORD_BREAK_PROP_BOTH_ALETTER_EXTPICT ||
		     skip.c == WORD_BREAK_PROP_HEBREW_LETTER) &&
		    len > 2 &&
		    (skip.a == WORD_BREAK_PROP_ALETTER              ||
		     skip.a == WORD_BREAK_PROP_BOTH_ALETTER_EXTPICT ||
		     skip.a == WORD_BREAK_PROP_HEBREW_LETTER)) {
			continue;
		}

		/* WB7a */
		if (skip.b == WORD_BREAK_PROP_HEBREW_LETTER &&
		    skip.c == WORD_BREAK_PROP_SINGLE_QUOTE) {
			continue;
		}

		/* WB7b */
		if (skip.b == WORD_BREAK_PROP_HEBREW_LETTER &&
		    skip.c == WORD_BREAK_PROP_DOUBLE_QUOTE &&
		    len > 2 &&
		    skip.d == WORD_BREAK_PROP_HEBREW_LETTER) {
			continue;
		}

		/* WB7c */
		if (skip.b == WORD_BREAK_PROP_DOUBLE_QUOTE &&
		    skip.c == WORD_BREAK_PROP_HEBREW_LETTER &&
		    off > 1 &&
		    skip.a == WORD_BREAK_PROP_HEBREW_LETTER) {
			continue;
		}

		/* WB8 */
		if (skip.b == WORD_BREAK_PROP_NUMERIC &&
		    skip.c == WORD_BREAK_PROP_NUMERIC) {
			continue;
		}

		/* WB9 */
		if ((skip.b == WORD_BREAK_PROP_ALETTER              ||
		     skip.b == WORD_BREAK_PROP_BOTH_ALETTER_EXTPICT ||
		     skip.b == WORD_BREAK_PROP_HEBREW_LETTER) &&
		    skip.c == WORD_BREAK_PROP_NUMERIC) {
			continue;
		}

		/* WB10 */
		if (skip.b == WORD_BREAK_PROP_NUMERIC &&
		    (skip.c == WORD_BREAK_PROP_ALETTER              ||
		     skip.c == WORD_BREAK_PROP_BOTH_ALETTER_EXTPICT ||
		     skip.c == WORD_BREAK_PROP_HEBREW_LETTER)) {
			continue;
		}

		/* WB11 */
		if ((skip.b == WORD_BREAK_PROP_MIDNUM       ||
		     skip.b == WORD_BREAK_PROP_MIDNUMLET    ||
		     skip.b == WORD_BREAK_PROP_SINGLE_QUOTE) &&
		    skip.c == WORD_BREAK_PROP_NUMERIC &&
		    off > 1 &&
		    skip.a == WORD_BREAK_PROP_NUMERIC) {
			continue;
		}

		/* WB12 */
		if (skip.b == WORD_BREAK_PROP_NUMERIC &&
		    (skip.c == WORD_BREAK_PROP_MIDNUM       ||
		     skip.c == WORD_BREAK_PROP_MIDNUMLET    ||
		     skip.c == WORD_BREAK_PROP_SINGLE_QUOTE) &&
		    len > 2 &&
		    skip.d == WORD_BREAK_PROP_NUMERIC) {
			continue;
		}

		/* WB13 */
		if (skip.b == WORD_BREAK_PROP_KATAKANA &&
		    skip.c == WORD_BREAK_PROP_KATAKANA) {
			continue;
		}

		/* WB13a */
		if ((skip.b == WORD_BREAK_PROP_ALETTER              ||
		     skip.b == WORD_BREAK_PROP_BOTH_ALETTER_EXTPICT ||
		     skip.b == WORD_BREAK_PROP_HEBREW_LETTER        ||
		     skip.b == WORD_BREAK_PROP_NUMERIC              ||
		     skip.b == WORD_BREAK_PROP_KATAKANA             ||
		     skip.b == WORD_BREAK_PROP_EXTENDNUMLET) &&
		    skip.c == WORD_BREAK_PROP_EXTENDNUMLET) {
			continue;
		}

		/* WB13b */
		if (skip.b == WORD_BREAK_PROP_EXTENDNUMLET &&
		    (skip.c == WORD_BREAK_PROP_ALETTER              ||
		     skip.c == WORD_BREAK_PROP_BOTH_ALETTER_EXTPICT ||
		     skip.c == WORD_BREAK_PROP_HEBREW_LETTER        ||
		     skip.c == WORD_BREAK_PROP_NUMERIC              ||
		     skip.c == WORD_BREAK_PROP_KATAKANA)) {
			continue;
		}

		/* WB15 and WB16 */
		if (!ri_even &&
		    skip.c == WORD_BREAK_PROP_REGIONAL_INDICATOR) {
			continue;
		}

		/* WB999 */
		break;
	}

	return off;
}

size_t
grapheme_next_word_break(const uint_least32_t *str, size_t len)
{
	return next_word_break(str, len, get_codepoint);
}

size_t
grapheme_next_word_break_utf8(const char *str, size_t len)
{
	return next_word_break(str, len, get_codepoint_utf8);
}
