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
 */

#define MAX_DEPTH 125

#include <stdio.h> /* ------------------------------------------------------------------ */
static size_t
determine_paragraph_level(const void *src, size_t srclen,
                          size_t (*get_codepoint)(const void *, size_t, size_t, uint_least32_t *),
                          size_t (*set_codepoint)(uint_least32_t, void *, size_t, size_t))
{
	enum bidi_property prop;
	size_t srcoff, isolate_level;
	uint_least32_t cp;

	for (srcoff = 0, isolate_level = 0; srcoff < srclen; ) {
		srcoff += get_codepoint(src, srclen, srcoff, &cp);
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

static size_t
handle_paragraph(const void *src, size_t srclen, enum grapheme_bidirectional_override override,
                 size_t (*get_codepoint)(const void *, size_t, size_t, uint_least32_t *),
                 size_t (*set_codepoint)(uint_least32_t, void *, size_t, size_t),
                 void *dest, size_t destlen)
{
	enum bidi_property prop;
	size_t srcoff, destoff, paragraph_level;

fprintf(stderr, "paragraph-call: par='%.*s'\n", (int)srclen, (const char *)src);
	/* determine paragraph level (rules P1-P3, HL1) */
	if (override == GRAPHEME_BIDIRECTIONAL_OVERRIDE_LTR) {
		paragraph_level = 0;
	} else if (override == GRAPHEME_BIDIRECTIONAL_OVERRIDE_RTL) {
		paragraph_level = 1;
	} else { /* GRAPHEME_BIDIRECTIONAL_OVERRIDE_NONE and invalid */
		paragraph_level = determine_paragraph_level(src, srclen,
		                                            get_codepoint,
		                                            set_codepoint);
	}
fprintf(stderr, "\tparagraph_level=%zu\n", paragraph_level);

	/* determine_explicit_levels(...); X1-X8 */
	/* prepare_implicit_processing(); X9-X10, BD13 */
	/* resolve_weak_types(); W1-W7 */
	/* resolve_neutral_and_isolate_formatting_types() N0-N2 */
	/* resolve_implicit_levels(); I1-I2 */
	/* reorder_resolved_levels(); L1-L4 */

	return destoff;
}

static size_t
logical_to_visual(const void *src, size_t srclen, enum grapheme_bidirectional_override override,
                  size_t (*get_codepoint)(const void *, size_t, size_t, uint_least32_t *),
                  size_t (*set_codepoint)(uint_least32_t, void *, size_t, size_t),
                  void *dest, size_t destlen)
{
	size_t srcoff, destoff, lastparoff;
	uint_least32_t cp;

	for (srcoff = destoff = lastparoff = 0; srcoff < srclen; ) {
		srcoff += get_codepoint(src, srclen, srcoff, &cp);

		/* P1 */
		if (get_bidi_property(cp) == BIDI_PROP_B ||
		    srcoff == srclen ||
		    (get_codepoint == get_codepoint_utf8 &&
		     srclen == SIZE_MAX && cp == 0)) {
			/*
			 * we encountered a paragraph separator or
			 * reached the end of the text.
			 * Call the paragraph handling function on
			 * the paragraph including the separator.
			 */
			if (get_codepoint == get_codepoint_utf8) {
				destoff += handle_paragraph((const char *)src + lastparoff,
				                            srcoff - lastparoff, override,
						            get_codepoint, set_codepoint,
				                            (char *)dest + destoff,
						            (destoff < destlen) ?
				                            (destlen - destoff) : 0);
			} else {
				destoff += handle_paragraph((const uint_least32_t *)src + lastparoff,
				                            srcoff - lastparoff, override,
				                            get_codepoint, set_codepoint,
				                            (uint_least32_t *)dest + destoff,
				                            (destoff < destlen) ?
				                            (destlen - destoff) : 0);
			}
			lastparoff = srcoff;
		}
	}

	return destoff;
}

size_t
grapheme_bidirectional_logical_to_visual(const uint_least32_t *src,
                                         size_t srclen,
                                         enum grapheme_bidirectional_override override,
                                         uint_least32_t *dest,
                                         size_t destlen)
{
	return logical_to_visual(src, srclen, override,
	                         get_codepoint, set_codepoint, dest, destlen);
}

size_t
grapheme_bidirectional_logical_to_visual_utf8(const char *src, size_t srclen,
                                              enum grapheme_bidirectional_override override,
                                              char *dest, size_t destlen)
{
	return logical_to_visual(src, srclen, override,
	                         get_codepoint_utf8, set_codepoint_utf8,
	                         dest, destlen);
}
