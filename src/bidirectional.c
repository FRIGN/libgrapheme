/* See LICENSE file for copyright and license details. */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "../gen/bidirectional.h"
#include "../grapheme.h"
#include "util.h"

static inline enum bidi_property
get_bidi_property(uint_least32_t cp)
{
	if (likely(cp <= 0x10FFFF)) {
		return (enum bidi_property)
		       bidi_minor[bidi_major[cp >> 8] + (cp & 0xff)];
	} else {
		return BIDI_PROP_L;
	}
}

/*
 * NOTES:
 * https://unicode.org/reports/tr9/
 * https://github.com/omid/Persian-Log2Vis/blob/master/bidi.php
 * https://github.com/fribidi/fribidi/blob/master/lib/fribidi.h
 *
 * Apply transformation separately
 * 	src, dest=1110001110000111 -> get contiguous blocks and apply
 * investigate fribidi and refactor API
 */

#define MAX_DEPTH 125

static uint8_t
determine_paragraph_level(const HERODOTUS_READER *r)
{
	HERODOTUS_READER tmp;
	enum bidi_property prop;
	uint8_t isolate_level;
	uint_least32_t cp;

	herodotus_reader_copy(r, &tmp);

	for (isolate_level = 0; herodotus_read_codepoint(&tmp, true, &cp) ==
	     HERODOTUS_STATUS_SUCCESS; ) {
		prop = get_bidi_property(cp);

		/* BD8/BD9 */
		if ((prop == BIDI_PROP_LRI ||
		     prop == BIDI_PROP_RLI ||
		     prop == BIDI_PROP_FSI) &&
		    isolate_level < MAX_DEPTH) {
			/* we hit an isolate initiator, increment counter */
			isolate_level++;
		} else if (prop == BIDI_PROP_PDI && isolate_level > 0) {
			isolate_level--;
		}

		/* P2 */
		if (isolate_level > 0) {
			continue;
		}

		/* P3 */
		if (prop == BIDI_PROP_L) {
			return 0;
		} else if (prop == BIDI_PROP_AL ||
		           prop == BIDI_PROP_R) {
			return 1;
		}
	}

	return 0;
}

static void
handle_paragraph(HERODOTUS_READER *r, enum grapheme_bidirectional_override override,
                 HERODOTUS_WRITER *w)
{
	enum bidi_property prop;
	uint8_t paragraph_level;

	/* determine paragraph level (rules P1-P3, HL1) */
	if (override == GRAPHEME_BIDIRECTIONAL_OVERRIDE_LTR) {
		paragraph_level = 0;
	} else if (override == GRAPHEME_BIDIRECTIONAL_OVERRIDE_RTL) {
		paragraph_level = 1;
	} else { /* GRAPHEME_BIDIRECTIONAL_OVERRIDE_NONE and invalid */
		paragraph_level = determine_paragraph_level(r);
	}

	/* determine_explicit_levels(...); X1-X8 */
	/* prepare_implicit_processing(); X9-X10, BD13 */
	/* resolve_weak_types(); W1-W7 */
	/* resolve_neutral_and_isolate_formatting_types() N0-N2 */
	/* resolve_implicit_levels(); I1-I2 */
	/* reorder_resolved_levels(); L1-L4 */
}

static size_t
next_paragraph_break(const HERODOTUS_READER *r)
{
	HERODOTUS_READER tmp;
	uint_least32_t cp;

	herodotus_reader_copy(r, &tmp);

	for ( ; herodotus_read_codepoint(&tmp, true, &cp) == HERODOTUS_STATUS_SUCCESS; ) {
		if (get_bidi_property(cp) == BIDI_PROP_B) {
			break;
		}
	}

	return herodotus_reader_number_read(&tmp);
}

static size_t
logical_to_visual(HERODOTUS_READER *r, enum grapheme_bidirectional_override override,
                  HERODOTUS_WRITER *w)
{
	size_t npb;

	for (; (npb = next_paragraph_break(r)) > 0;) {
		/* P1 */
		herodotus_reader_push_advance_limit(r, npb);
		handle_paragraph(r, override, w);
		herodotus_reader_pop_limit(r);
	}

	return herodotus_writer_number_written(w);
}

size_t
grapheme_bidirectional_logical_to_visual(const uint_least32_t *src,
                                         size_t srclen,
                                         enum grapheme_bidirectional_override override,
                                         uint_least32_t *dest,
                                         size_t destlen)
{
	HERODOTUS_READER r;
	HERODOTUS_WRITER w;

	herodotus_reader_init(&r, HERODOTUS_TYPE_CODEPOINT, src, srclen);
	herodotus_writer_init(&w, HERODOTUS_TYPE_CODEPOINT, dest, destlen);

	return logical_to_visual(&r, override, &w);
}

size_t
grapheme_bidirectional_logical_to_visual_utf8(const char *src, size_t srclen,
                                              enum grapheme_bidirectional_override override,
                                              char *dest, size_t destlen)
{
	HERODOTUS_READER r;
	HERODOTUS_WRITER w;

	herodotus_reader_init(&r, HERODOTUS_TYPE_UTF8, src, srclen);
	herodotus_writer_init(&w, HERODOTUS_TYPE_UTF8, dest, destlen);

	return logical_to_visual(&r, override, &w);
}
