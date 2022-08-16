/* See LICENSE file for copyright and license details. */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "../gen/sentence.h"
#include "../grapheme.h"
#include "util.h"

static inline enum sentence_break_property
get_break_prop(uint_least32_t cp)
{
	if (likely(cp <= 0x10FFFF)) {
		return (enum sentence_break_property)
		       sentence_break_minor[sentence_break_major[cp >> 8] +
		       (cp & 0xff)];
	} else {
		return SENTENCE_BREAK_PROP_OTHER;
	}
}

static size_t
next_sentence_break(const void *str, size_t len, size_t (*get_codepoint)
                    (const void *, size_t, size_t, uint_least32_t *))
{
	struct {
		enum sentence_break_property a, b, c, d;
	} raw, skip;
	enum sentence_break_property res;
	uint_least32_t cp;
	uint_least8_t aterm_close_sp_level = 0,
	              saterm_close_sp_parasep_level = 0;
	size_t off, tmp, new_off;

	/* check degenerate cases */
	if (str == NULL || len == 0) {
		return 0;
	}

	/*
	 * Apply sentence breaking algorithm (UAX #29), see
	 * https://unicode.org/reports/tr29/#Sentence_Boundary_Rules
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
	 * half of the rules (after SB5). Thus, we do not only have
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
	raw.b = NUM_SENTENCE_BREAK_PROPS;
	if ((off = get_codepoint(str, len, 0, &cp)) >= len) {
		return 1;
	}
	raw.c = get_break_prop(cp);
	(void)get_codepoint(str, len, off, &cp);
	raw.d = get_break_prop(cp);
	skip.a = skip.b = NUM_SENTENCE_BREAK_PROPS;

	for (; off < len; off = new_off) {
		/*
		 * Update left side (a and b) of the skip state by
		 * "shifting in" the raw.c property as long as it is
		 * not one of the "ignored" character properties.
		 * While at it, update the RI-counter.
		 *
		 */
		if (raw.c != SENTENCE_BREAK_PROP_EXTEND &&
		    raw.c != SENTENCE_BREAK_PROP_FORMAT) {
		    	skip.a = skip.b;
			skip.b = raw.c;

			/*
			 * Here comes a bit of magic. The rules
			 * SB8, SB8a, SB9 and SB10 have very complicated
			 * left-hand-side-rules of the form
			 *
			 *  ATerm Close* Sp*
			 *  SATerm Close*
			 *  SATerm Close* Sp*
			 *  SATerm Close* Sp* ParaSep?
			 * 
			 * but instead of backtracking, we keep the
			 * state as some kind of "power level" in
			 * two variables
			 *
			 *  aterm_close_sp_level
			 *  saterm_close_sp_parasep_level
			 * 
			 * that go from 0 to 3/4:
			 *
			 *  0: we are not in the sequence
			 *  1: we have one ATerm/SATerm to the left of
			 *     the middle spot
			 *  2: we have one ATerm/SATerm and one or more
			 *     Close to the left of the middle spot
			 *  3: we have one ATerm/SATerm, zero or more
			 *     Close and one or more Sp to the left of
			 *     the middle spot.
			 *  4: we have one SATerm, zero or more Close,
			 *     zero or more Sp and one ParaSep to the
			 *     left of the middle spot.
			 *
			 */
			if ((aterm_close_sp_level == 0 ||
			     aterm_close_sp_level == 1) &&
			    skip.b == SENTENCE_BREAK_PROP_ATERM) {
			    	/* sequence has begun */
				aterm_close_sp_level = 1;
			} else if ((aterm_close_sp_level == 1 ||
			            aterm_close_sp_level == 2) &&
			           skip.b == SENTENCE_BREAK_PROP_CLOSE) {
				/* close-sequence begins or continued */
				aterm_close_sp_level = 2;
			} else if ((aterm_close_sp_level == 1 ||
			            aterm_close_sp_level == 2 ||
				    aterm_close_sp_level == 3) &&
			           skip.b == SENTENCE_BREAK_PROP_SP) {
				/* sp-sequence begins or continued */
				aterm_close_sp_level = 3;
			} else {
				/* sequence broke */
				aterm_close_sp_level = 0;
			}

			if ((saterm_close_sp_parasep_level == 0 ||
			     saterm_close_sp_parasep_level == 1) &&
			    (skip.b == SENTENCE_BREAK_PROP_STERM ||
			     skip.b == SENTENCE_BREAK_PROP_ATERM)) {
			    	/* sequence has begun */
				saterm_close_sp_parasep_level = 1;
			} else if ((saterm_close_sp_parasep_level == 1 ||
			            saterm_close_sp_parasep_level == 2) &&
			           skip.b == SENTENCE_BREAK_PROP_CLOSE) {
				/* close-sequence begins or continued */
				saterm_close_sp_parasep_level = 2;
			} else if ((saterm_close_sp_parasep_level == 1 ||
			            saterm_close_sp_parasep_level == 2 ||
				    saterm_close_sp_parasep_level == 3) &&
			           skip.b == SENTENCE_BREAK_PROP_SP) {
				/* sp-sequence begins or continued */
				saterm_close_sp_parasep_level = 3;
			} else if ((saterm_close_sp_parasep_level == 1 ||
			            saterm_close_sp_parasep_level == 2 ||
			            saterm_close_sp_parasep_level == 3) &&
			           (skip.b == SENTENCE_BREAK_PROP_SEP ||
			            skip.b == SENTENCE_BREAK_PROP_CR  ||
			            skip.b == SENTENCE_BREAK_PROP_LF)) {
				/* ParaSep at the end of the sequence */
				saterm_close_sp_parasep_level = 4;
			} else {
				/* sequence broke */
				saterm_close_sp_parasep_level = 0;
			}
		}

		/*
		 * Update right side (b and c) of the skip state by
		 * starting at the breakpoint and detecting the two
		 * following non-ignored character classes
		 *
		 */
		skip.c = NUM_SENTENCE_BREAK_PROPS;
		for (tmp = off; tmp < len; ) {
			tmp += get_codepoint(str, len, tmp, &cp);
			res = get_break_prop(cp);

			if (res != SENTENCE_BREAK_PROP_EXTEND &&
			    res != SENTENCE_BREAK_PROP_FORMAT) {
				skip.c = res;
				break;
			}
		}
		skip.d = NUM_SENTENCE_BREAK_PROPS;
		for (; tmp < len; ) {
			tmp += get_codepoint(str, len, tmp, &cp);
			res = get_break_prop(cp);

			if (res != SENTENCE_BREAK_PROP_EXTEND &&
			    res != SENTENCE_BREAK_PROP_FORMAT) {
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
			raw.d = NUM_SENTENCE_BREAK_PROPS;
		}

		/* SB3 */
		if (raw.b == SENTENCE_BREAK_PROP_CR &&
		    raw.c == SENTENCE_BREAK_PROP_LF) {
			continue;
		}

		/* SB4 */
		if (raw.b == SENTENCE_BREAK_PROP_SEP ||
		    raw.b == SENTENCE_BREAK_PROP_CR  ||
		    raw.b == SENTENCE_BREAK_PROP_LF) {
			break;
		}

		/* SB5 */
		if (raw.c == SENTENCE_BREAK_PROP_EXTEND ||
		    raw.c == SENTENCE_BREAK_PROP_FORMAT) {
			continue;
		}

		/* SB6 */
		if (skip.b == SENTENCE_BREAK_PROP_ATERM &&
		    skip.c == SENTENCE_BREAK_PROP_NUMERIC) {
			continue;
		}

		/* SB7 */
		if (off > 1 &&
		    (skip.a == SENTENCE_BREAK_PROP_UPPER ||
		     skip.a == SENTENCE_BREAK_PROP_LOWER) &&
		    skip.b == SENTENCE_BREAK_PROP_ATERM &&
		    skip.c == SENTENCE_BREAK_PROP_UPPER) {
			continue;
		}

		/* SB8 */
		if (aterm_close_sp_level == 1 ||
		    aterm_close_sp_level == 2 ||
		    aterm_close_sp_level == 3) {
			/*
			 * This is the most complicated rule, requiring
			 * the right-hand-side to satisfy the regular expression
			 *
			 *  ( ¬(OLetter | Upper | Lower | ParaSep | SATerm) )* Lower
			 *
			 * which we simply check "manually" given LUT-lookups
			 * are very cheap.
			 *
			 */
			for (tmp = off, res = NUM_SENTENCE_BREAK_PROPS; tmp < len; ) {
				tmp += get_codepoint(str, len, tmp, &cp);
				res = get_break_prop(cp);

				if (res == SENTENCE_BREAK_PROP_OLETTER ||
				    res == SENTENCE_BREAK_PROP_UPPER   ||
				    res == SENTENCE_BREAK_PROP_LOWER   ||
				    res == SENTENCE_BREAK_PROP_SEP     ||
				    res == SENTENCE_BREAK_PROP_CR      ||
				    res == SENTENCE_BREAK_PROP_LF      ||
				    res == SENTENCE_BREAK_PROP_STERM   ||
				    res == SENTENCE_BREAK_PROP_ATERM) {
					break;
				}
			}

			if (res == SENTENCE_BREAK_PROP_LOWER) {
				continue;
			}
		}

		/* SB8a */
		if ((saterm_close_sp_parasep_level == 1 ||
		     saterm_close_sp_parasep_level == 2 ||
		     saterm_close_sp_parasep_level == 3) &&
		    (skip.c == SENTENCE_BREAK_PROP_SCONTINUE ||
		     skip.c == SENTENCE_BREAK_PROP_STERM     ||
                     skip.c == SENTENCE_BREAK_PROP_ATERM)) {
			continue;
		}

		/* SB9 */
		if ((saterm_close_sp_parasep_level == 1 ||
		     saterm_close_sp_parasep_level == 2) &&
		    (skip.c == SENTENCE_BREAK_PROP_CLOSE ||
		     skip.c == SENTENCE_BREAK_PROP_SP    ||
		     skip.c == SENTENCE_BREAK_PROP_SEP   ||
		     skip.c == SENTENCE_BREAK_PROP_CR    ||
		     skip.c == SENTENCE_BREAK_PROP_LF)) {
			continue;
		}

		/* SB10 */
		if ((saterm_close_sp_parasep_level == 1 ||
		     saterm_close_sp_parasep_level == 2 ||
		     saterm_close_sp_parasep_level == 3) &&
		    (skip.c == SENTENCE_BREAK_PROP_SP  ||
		     skip.c == SENTENCE_BREAK_PROP_SEP ||
		     skip.c == SENTENCE_BREAK_PROP_CR  ||
		     skip.c == SENTENCE_BREAK_PROP_LF)) {
			continue;
		}

		/* SB11 */
		if (saterm_close_sp_parasep_level == 1 ||
		    saterm_close_sp_parasep_level == 2 ||
		    saterm_close_sp_parasep_level == 3 ||
		    saterm_close_sp_parasep_level == 4) {
			break;
		}

		/* SB998 */
		continue;
	}

	return off;
}

size_t
grapheme_next_sentence_break(const uint_least32_t *str, size_t len)
{
	return next_sentence_break(str, len, get_codepoint);
}

size_t
grapheme_next_sentence_break_utf8(const char *str, size_t len)
{
	return next_sentence_break(str, len, get_codepoint_utf8);
}
