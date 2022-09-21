/* See LICENSE file for copyright and license details. */
#include <stdint.h>

#include "../grapheme.h"
#include "../gen/case.h"
#include "util.h"

static inline enum case_property
get_case_property(uint_least32_t cp)
{
	if (likely(cp <= 0x10FFFF)) {
		return (enum case_property)
		       case_minor[case_major[cp >> 8] + (cp & 0xff)];
	} else {
		return CASE_PROP_OTHER;
	}
}

static inline int_least32_t
get_case_offset(uint_least32_t cp, const uint_least16_t *major,
                const int_least32_t *minor)
{
	if (likely(cp <= 0x10FFFF)) {
		/*
		 * this value might be larger than or equal to 0x110000
		 * for the special-case-mapping. This needs to be handled
		 * separately
		 */
		return minor[major[cp >> 8] + (cp & 0xff)];
	} else {
		return 0;
	}
}

static inline size_t
to_case(HERODOTUS_READER *r, HERODOTUS_WRITER *w,
        uint_least8_t final_sigma_level, const uint_least16_t *major,
        const int_least32_t *minor, const struct special_case *sc)
{
	HERODOTUS_READER tmp;
	enum case_property prop;
	enum herodotus_status s;
	size_t off, i;
	uint_least32_t cp, tmp_cp;
	int_least32_t map;

	for (; herodotus_read_codepoint(r, true, &cp) == HERODOTUS_STATUS_SUCCESS;) {
		if (sc == lower_special) {
			/*
			 * For the special Final_Sigma-rule (see SpecialCasing.txt),
			 * which is the only non-localized case-dependent rule,
			 * we apply a different mapping when a sigma is at the
			 * end of a word.
			 *
			 * Before: cased case-ignorable*
			 * After: not(case-ignorable* cased)
			 *
			 * We check the after-condition on demand, but the before-
			 * condition is best checked using the "level"-heuristic
			 * also used in the sentence and line breaking-implementations.
			 */
			if (cp == UINT32_C(0x03A3) && /* GREEK CAPITAL LETTER SIGMA */
			    (final_sigma_level == 1 ||
			     final_sigma_level == 2)) {
				/*
				 * check succeeding characters by first skipping
				 * all case-ignorable characters and then checking
				 * if the succeeding character is cased, invalidating
				 * the after-condition
				 */
				herodotus_reader_copy(r, &tmp);
				for (prop = NUM_CASE_PROPS;
				     (s = herodotus_read_codepoint(&tmp, true, &tmp_cp)) ==
				     HERODOTUS_STATUS_SUCCESS; ) {
					prop = get_case_property(tmp_cp);

					if (prop != CASE_PROP_CASE_IGNORABLE &&
					    prop != CASE_PROP_BOTH_CASED_CASE_IGNORABLE) {
					    	break;
					}
				}

				/*
				 * Now prop is something other than case-ignorable or
				 * the source-string ended.
				 * If it is something other than cased, we know
				 * that the after-condition holds
				 */
				if (s != HERODOTUS_STATUS_SUCCESS ||
				    (prop != CASE_PROP_CASED &&
				     prop != CASE_PROP_BOTH_CASED_CASE_IGNORABLE)) {
					/*
					 * write GREEK SMALL LETTER FINAL SIGMA to
					 * destination
					 */
					herodotus_write_codepoint(w, UINT32_C(0x03C2));
					
					/* reset Final_Sigma-state and continue */
					final_sigma_level = 0;
					continue;
				}
			}

			/* update state */
			prop = get_case_property(cp);
			if ((final_sigma_level == 0 ||
			     final_sigma_level == 1) &&
			    (prop == CASE_PROP_CASED ||
			     prop == CASE_PROP_BOTH_CASED_CASE_IGNORABLE)) {
				/* sequence has begun */
				final_sigma_level = 1;
			} else if ((final_sigma_level == 1 ||
			            final_sigma_level == 2) &&
			           (prop == CASE_PROP_CASE_IGNORABLE ||
			            prop == CASE_PROP_BOTH_CASED_CASE_IGNORABLE)) {
				/* case-ignorable sequence begins or continued */
				final_sigma_level = 2;
			} else {
				/* sequence broke */
				final_sigma_level = 0;
			}
		}

		/* get and handle case mapping */
		if (unlikely((map = get_case_offset(cp, major, minor)) >=
		             INT32_C(0x110000))) {
			/* we have a special case and the offset in the sc-array
			 * is the difference to 0x110000*/
			off = (uint_least32_t)map - UINT32_C(0x110000);

			for (i = 0; i < sc[off].cplen; i++) {
				herodotus_write_codepoint(w, sc[off].cp[i]);
			}
		} else {
			/* we have a simple mapping */
			herodotus_write_codepoint(w, (uint_least32_t)
			                          ((int_least32_t)cp + map));
		}
	}

	herodotus_writer_nul_terminate(w);

	return herodotus_writer_number_written(w);
}

static size_t
herodotus_next_word_break(const HERODOTUS_READER *r)
{
	if (r->src == NULL || r->off > r->srclen) {
		return 0;
	}

	if (r->type == HERODOTUS_TYPE_CODEPOINT) {
		return grapheme_next_word_break(
			((const uint_least32_t *)(r->src)) + r->off,
			r->srclen - r->off);
	} else { /* r->type == HERODOTUS_TYPE_UTF8 */
		return grapheme_next_word_break_utf8(
			((const char *)(r->src)) + r->off,
			r->srclen - r->off);
	}
}

static inline size_t
to_titlecase(HERODOTUS_READER *r, HERODOTUS_WRITER *w)
{
	enum case_property prop;
	enum herodotus_status s;
	uint_least32_t cp;

	for (;;) {
		herodotus_reader_push_advance_limit(r, herodotus_next_word_break(r));
		for (; (s = herodotus_read_codepoint(r, false, &cp)) == HERODOTUS_STATUS_SUCCESS;) {
			/* check if we have a cased character */
			prop = get_case_property(cp);
			if (prop == CASE_PROP_CASED ||
			    prop == CASE_PROP_BOTH_CASED_CASE_IGNORABLE) {
				break;
			} else {
				/* write the data to the output verbatim, it if permits */
				herodotus_write_codepoint(w, cp);

				/* increment reader */
				herodotus_read_codepoint(r, true, &cp);
			}
		}

		if (s == HERODOTUS_STATUS_END_OF_BUFFER) {
			/* we are done */
			break;
		} else if (s == HERODOTUS_STATUS_SOFT_LIMIT_REACHED) {
			/*
			 * we did not encounter any cased character
			 * up to the word break
			 */
			continue;
		} else {
			/*
			 * we encountered a cased character before the word
			 * break, convert it to titlecase
			 */
			herodotus_reader_push_advance_limit(r,
				herodotus_reader_next_codepoint_break(r));
			to_case(r, w, 0, title_major, title_minor, title_special);
			herodotus_reader_pop_limit(r);
		}

		/* cast the rest of the codepoints in the word to lowercase */
		to_case(r, w, 1, lower_major, lower_minor, lower_special);

		herodotus_reader_pop_limit(r);
	}

	herodotus_writer_nul_terminate(w);

	return herodotus_writer_number_written(w);
}

size_t
grapheme_to_uppercase(const uint_least32_t *src, size_t srclen, uint_least32_t *dest, size_t destlen)
{
	HERODOTUS_READER r;
	HERODOTUS_WRITER w;

	herodotus_reader_init(&r, HERODOTUS_TYPE_CODEPOINT, src, srclen);
	herodotus_writer_init(&w, HERODOTUS_TYPE_CODEPOINT, dest, destlen);

	return to_case(&r, &w, 0, upper_major, upper_minor, upper_special);
}

size_t
grapheme_to_lowercase(const uint_least32_t *src, size_t srclen, uint_least32_t *dest, size_t destlen)
{
	HERODOTUS_READER r;
	HERODOTUS_WRITER w;

	herodotus_reader_init(&r, HERODOTUS_TYPE_CODEPOINT, src, srclen);
	herodotus_writer_init(&w, HERODOTUS_TYPE_CODEPOINT, dest, destlen);

	return to_case(&r, &w, 0, lower_major, lower_minor, lower_special);
}

size_t
grapheme_to_titlecase(const uint_least32_t *src, size_t srclen, uint_least32_t *dest, size_t destlen)
{
	HERODOTUS_READER r;
	HERODOTUS_WRITER w;

	herodotus_reader_init(&r, HERODOTUS_TYPE_CODEPOINT, src, srclen);
	herodotus_writer_init(&w, HERODOTUS_TYPE_CODEPOINT, dest, destlen);

	return to_titlecase(&r, &w);
}

size_t
grapheme_to_uppercase_utf8(const char *src, size_t srclen, char *dest, size_t destlen)
{
	HERODOTUS_READER r;
	HERODOTUS_WRITER w;

	herodotus_reader_init(&r, HERODOTUS_TYPE_UTF8, src, srclen);
	herodotus_writer_init(&w, HERODOTUS_TYPE_UTF8, dest, destlen);

	return to_case(&r, &w, 0, upper_major, upper_minor, upper_special);
}

size_t
grapheme_to_lowercase_utf8(const char *src, size_t srclen, char *dest, size_t destlen)
{
	HERODOTUS_READER r;
	HERODOTUS_WRITER w;

	herodotus_reader_init(&r, HERODOTUS_TYPE_UTF8, src, srclen);
	herodotus_writer_init(&w, HERODOTUS_TYPE_UTF8, dest, destlen);

	return to_case(&r, &w, 0, lower_major, lower_minor, lower_special);
}

size_t
grapheme_to_titlecase_utf8(const char *src, size_t srclen, char *dest, size_t destlen)
{
	HERODOTUS_READER r;
	HERODOTUS_WRITER w;

	herodotus_reader_init(&r, HERODOTUS_TYPE_UTF8, src, srclen);
	herodotus_writer_init(&w, HERODOTUS_TYPE_UTF8, dest, destlen);

	return to_titlecase(&r, &w);
}

static inline bool
is_case(const void *src, size_t srclen,
        size_t srcnumprocess,
        size_t (*get_codepoint)(const void *, size_t, size_t, uint_least32_t *),
        const uint_least16_t *major, const int_least32_t *minor,
        const struct special_case *sc, size_t *output)
{
	size_t srcoff, new_srcoff, tmp, res, off, i;
	uint_least32_t cp, tmp_cp;
	int_least32_t map;

	for (srcoff = 0; srcoff < srcnumprocess; srcoff = new_srcoff) {
		/* read in next source codepoint */
		new_srcoff = srcoff + get_codepoint(src, srclen, srcoff, &cp);

		/* get and handle case mapping */
		if (unlikely((map = get_case_offset(cp, major, minor)) >=
		             INT32_C(0x110000))) {
			/* we have a special case and the offset in the sc-array
			 * is the difference to 0x110000*/
			off = (uint_least32_t)map - UINT32_C(0x110000);

			for (i = 0, tmp = srcoff; i < sc[off].cplen; i++, tmp += res) {
				res = get_codepoint(src, srclen, srcoff, &tmp_cp);
				if (tmp_cp != sc[off].cp[i]) {
					/* we have a difference */
					if (output) {
						*output = tmp;
					}
					return false;
				}
			}
			new_srcoff = tmp;
		} else {
			/* we have a simple mapping */
			if (cp != (uint_least32_t)((int_least32_t)cp + map)) {
				/* we have a difference */
				if (output) {
					*output = srcoff;
				}
				return false;
			}
		}
	}

	if (output) {
		*output = srcoff;
	}
	return true;
}

static inline bool
is_titlecase(const void *src, size_t srclen,
             size_t (*get_codepoint)(const void *, size_t, size_t, uint_least32_t *),
	     size_t *output)
{
	enum case_property prop;
	size_t next_wb, srcoff, res, tmp_output;
	uint_least32_t cp;

	for (srcoff = 0; ; ) {
		if (get_codepoint == get_codepoint_utf8) {
			if ((next_wb = grapheme_next_word_break_utf8((const char *)src + srcoff,
			                                             srclen - srcoff)) == 0) {
				/* we consumed all of the string */
				break;
			}
		} else {
			if ((next_wb = grapheme_next_word_break((const uint_least32_t *)src + srcoff,
			                                        srclen - srcoff)) == 0) {
				/* we consumed all of the string */
				break;
			}
		}

		for (; next_wb > 0 && srcoff < srclen; next_wb -= res, srcoff += res) {
			/* check if we have a cased character */
			res = get_codepoint(src, srclen, srcoff, &cp);
			prop = get_case_property(cp);
			if (prop == CASE_PROP_CASED ||
			    prop == CASE_PROP_BOTH_CASED_CASE_IGNORABLE) {
				break;
			}
		}

		if (next_wb > 0) {
			/* get character length */
			res = get_codepoint(src, srclen, srcoff, &cp);

			/* we have a cased character at srcoff, check if it's titlecase */
			if (get_codepoint == get_codepoint_utf8) {
				if (!is_case((const char *)src + srcoff,
				              srclen - srcoff, res,
					      get_codepoint_utf8, title_major,
			                      title_minor, title_special, &tmp_output)) {
					if (output) {
						*output = srcoff + tmp_output;
					}
					return false;
				}
			} else {
				if (!is_case((const uint_least32_t *)src + srcoff,
				              srclen - srcoff, res,
					      get_codepoint, title_major,
			                      title_minor, title_special, &tmp_output)) {
					if (output) {
						*output = srcoff + tmp_output;
					}
					return false;
				}
			}

			/*
			 * we consumed a character (make sure to never
			 * underflow next_wb; this should not happen,
			 * but it's better to be sure)
			 */
			srcoff += res;
			next_wb -= (res <= next_wb) ? res : next_wb;
		}

		/* check if the rest of the codepoints in the word are lowercase */
		if (get_codepoint == get_codepoint_utf8) {
			if (!is_case((const char *)src + srcoff,
			              srclen - srcoff, next_wb,
				      get_codepoint_utf8, lower_major,
		                      lower_minor, lower_special, &tmp_output)) {
				if (output) {
					*output = srcoff + tmp_output;
				}
				return false;
			}
		} else {
			if (!is_case((const uint_least32_t *)src + srcoff,
			              srclen - srcoff, next_wb,
				      get_codepoint, lower_major,
		                      lower_minor, lower_special, &tmp_output)) {
				if (output) {
					*output = srcoff + tmp_output;
				}
				return false;
			}
		}
		srcoff += next_wb;
	}

	if (output) {
		*output = srcoff;
	}
	return true;
}

bool
grapheme_is_uppercase(const uint_least32_t *src, size_t srclen, size_t *caselen)
{
	return is_case(src, srclen, srclen, get_codepoint,
	               upper_major, upper_minor, upper_special, caselen);
}

bool
grapheme_is_lowercase(const uint_least32_t *src, size_t srclen, size_t *caselen)
{
	return is_case(src, srclen, srclen, get_codepoint,
	               lower_major, lower_minor, lower_special, caselen);
}

bool
grapheme_is_titlecase(const uint_least32_t *src, size_t srclen, size_t *caselen)
{
	return is_titlecase(src, srclen, get_codepoint, caselen);
}

bool
grapheme_is_uppercase_utf8(const char *src, size_t srclen, size_t *caselen)
{
	return is_case(src, srclen, srclen, get_codepoint_utf8,
	               upper_major, upper_minor, upper_special, caselen);
}

bool
grapheme_is_lowercase_utf8(const char *src, size_t srclen, size_t *caselen)
{
	return is_case(src, srclen, srclen, get_codepoint_utf8,
	               lower_major, lower_minor, lower_special, caselen);

}

bool
grapheme_is_titlecase_utf8(const char *src, size_t srclen, size_t *caselen)
{
	return is_titlecase(src, srclen, get_codepoint_utf8, caselen);
}
