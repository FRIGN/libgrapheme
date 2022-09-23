/* See LICENSE file for copyright and license details. */
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../gen/types.h"
#include "../grapheme.h"
#include "util.h"

void
herodotus_reader_init(HERODOTUS_READER *r, enum herodotus_type type,
                      const void *src, size_t srclen)
{
	size_t i;

	r->type = type;
	r->src = src;
	r->srclen = srclen;
	r->off = 0;
	r->terminated_by_null = false;

	for (i = 0; i < LEN(r->soft_limit); i++) {
		r->soft_limit[i] = SIZE_MAX;
	}
}

void
herodotus_reader_copy(const HERODOTUS_READER *src, HERODOTUS_READER *dest)
{
	size_t i;

	dest->type = src->type;
	dest->src = src->src;
	dest->srclen = src->srclen;
	dest->off = src->off;
	dest->terminated_by_null = src->terminated_by_null;

	for (i = 0; i < LEN(src->soft_limit); i++) {
		dest->soft_limit[i] = src->soft_limit[i];
	}
}

void
herodotus_reader_push_advance_limit(HERODOTUS_READER *r, size_t count)
{
	size_t i;

	for (i = LEN(r->soft_limit) - 1; i >= 1; i--) {
		r->soft_limit[i] = r->soft_limit[i - 1];
	}
	r->soft_limit[0] = r->off + count;
}

void
herodotus_reader_pop_limit(HERODOTUS_READER *r)
{
	size_t i;

	for (i = 0; i < LEN(r->soft_limit) - 1; i++) {
		r->soft_limit[i] = r->soft_limit[i + 1];
	}
	r->soft_limit[LEN(r->soft_limit) - 1] = SIZE_MAX;
}

size_t
herodotus_reader_next_word_break(const HERODOTUS_READER *r)
{
	if (r->type == HERODOTUS_TYPE_CODEPOINT) {
		return grapheme_next_word_break(
			(const uint_least32_t *)(r->src) + r->off,
			MIN(r->srclen, r->soft_limit[0]) - r->off);
	} else { /* r->type == HERODOTUS_TYPE_UTF8 */
		return grapheme_next_word_break_utf8(
			(const char *)(r->src) + r->off,
			MIN(r->srclen, r->soft_limit[0]) - r->off);
	}
}

size_t
herodotus_reader_next_codepoint_break(const HERODOTUS_READER *r)
{
	if (r->type == HERODOTUS_TYPE_CODEPOINT) {
		return (r->off < MIN(r->srclen, r->soft_limit[0])) ? 1 : 0;
	} else { /* r->type == HERODOTUS_TYPE_UTF8 */
		return grapheme_decode_utf8(
			(const char *)(r->src) + r->off,
			MIN(r->srclen, r->soft_limit[0]) - r->off, NULL);
	}
}

size_t
herodotus_reader_number_read(const HERODOTUS_READER *r)
{
	return r->off;
}

enum herodotus_status
herodotus_read_codepoint(HERODOTUS_READER *r, bool advance, uint_least32_t *cp)
{
	size_t ret;

	if (r->terminated_by_null || r->off >= r->srclen || r->src == NULL) {
		*cp = GRAPHEME_INVALID_CODEPOINT;
		return HERODOTUS_STATUS_END_OF_BUFFER;
	}

	if (r->off >= r->soft_limit[0]) {
		*cp = GRAPHEME_INVALID_CODEPOINT;
		return HERODOTUS_STATUS_SOFT_LIMIT_REACHED;
	}

	if (r->type == HERODOTUS_TYPE_CODEPOINT) {
		*cp = ((const uint_least32_t *)(r->src))[r->off++];
	} else { /* r->type == HERODOTUS_TYPE_UTF8 */
		ret = grapheme_decode_utf8((const char *)r->src + r->off,
		                           MIN(r->srclen, r->soft_limit[0]) -
		                           r->off, cp);

		if (unlikely(r->srclen == SIZE_MAX && *cp == 0)) {
			/*
			 * We encountered a NUL-byte. Don't increment
			 * offset and return as if the buffer had ended
			 * here all along
			 */
			r->terminated_by_null = true;
			return HERODOTUS_STATUS_END_OF_BUFFER;
		}

		if (r->off + ret > MIN(r->srclen, r->soft_limit[0])) {
			/*
			 * we want more than we have; instead of
			 * returning garbage we terminate here.
			 */
			return HERODOTUS_STATUS_END_OF_BUFFER;
		}

		/*
		 * Increase offset which we now know won't surpass
		 * the limits, unless we got told otherwise
		 */
		if (advance) {
			r->off += ret;
		}
	}

	return HERODOTUS_STATUS_SUCCESS;
}

void
herodotus_writer_init(HERODOTUS_WRITER *w, enum herodotus_type type,
                      void *dest, size_t destlen)
{
	w->type = type;
	w->dest = dest;
	w->destlen = destlen;
	w->off = 0;
	w->first_unwritable_offset = SIZE_MAX;
}

void
herodotus_writer_nul_terminate(HERODOTUS_WRITER *w)
{
	if (w->dest == NULL) {
		return;
	}

	if (w->off < w->destlen) {
		/* We still have space in the buffer. Simply use it */
		if (w->type == HERODOTUS_TYPE_CODEPOINT) {
			((uint_least32_t *)(w->dest))[w->off] = 0;
		} else { /* w->type == HERODOTUS_TYPE_UTF8 */
			((char *)(w->dest))[w->off] = '\0';
		}
	} else if (w->first_unwritable_offset < w->destlen) {
		/*
		 * There is no more space in the buffer. However,
		 * we have noted down the first offset we couldn't
		 * use to write into the buffer and it's smaller than
		 * destlen. Thus we bailed writing into the
		 * destination when a multibyte-codepoint couldn't be
		 * written. So the last "real" byte might be at
		 * destlen-4, destlen-3, destlen-2 or destlen-1
		 * (the last case meaning truncation).
		 */
		if (w->type == HERODOTUS_TYPE_CODEPOINT) {
			((uint_least32_t *)(w->dest))
				[w->first_unwritable_offset] = 0;
		} else { /* w->type == HERODOTUS_TYPE_UTF8 */
			((char *)(w->dest))[w->first_unwritable_offset] = '\0';
		}
	} else {
		/*
		 * In this case, there is no more space in the buffer and
		 * the last unwritable offset is larger than
		 * or equal to the destination buffer length. This means
		 * that we are forced to simply write into the last
		 * byte.
		 */
		if (w->type == HERODOTUS_TYPE_CODEPOINT) {
			((uint_least32_t *)(w->dest))
				[w->destlen - 1] = 0;
		} else { /* w->type == HERODOTUS_TYPE_UTF8 */
			((char *)(w->dest))[w->destlen - 1] = '\0';
		}
	}

	/* w->off is not incremented in any case */
}

size_t
herodotus_writer_number_written(const HERODOTUS_WRITER *w)
{
	return w->off;
}

void
herodotus_write_codepoint(HERODOTUS_WRITER *w, uint_least32_t cp)
{
	size_t ret;

	/*
	 * This function will always faithfully say how many codepoints
	 * were written, even if the buffer ends. This is used to enable
	 * truncation detection.
	 */
	if (w->type == HERODOTUS_TYPE_CODEPOINT) {
		if (w->dest != NULL && w->off < w->destlen) {
			((uint_least32_t *)(w->dest))[w->off] = cp;
		}

		w->off += 1;
	} else { /* w->type == HERODOTUS_TYPE_UTF8 */
		/*
		 * First determine how many bytes we need to encode the
		 * codepoint
		 */
		ret = grapheme_encode_utf8(cp, NULL, 0);

		if (w->dest != NULL && w->off + ret < w->destlen) {
			/* we still have enough room in the buffer */
			grapheme_encode_utf8(cp, (char *)(w->dest) +
			                     w->off, w->destlen - w->off);
		} else if (w->first_unwritable_offset == SIZE_MAX) {
			/*
			 * the first unwritable offset has not been
			 * noted down, so this is the first time we can't
			 * write (completely) to an offset
			 */
			w->first_unwritable_offset = w->off;
		}

		w->off += ret;
	}
}

inline size_t
get_codepoint(const void *str, size_t len, size_t offset, uint_least32_t *cp)
{
	if (offset < len) {
		*cp = ((const uint_least32_t *)str)[offset];
		return 1;
	} else {
		*cp = GRAPHEME_INVALID_CODEPOINT;
		return 0;
	}
}

inline size_t
get_codepoint_utf8(const void *str, size_t len, size_t offset, uint_least32_t *cp)
{
	size_t ret;

	if (offset < len) {
		ret = grapheme_decode_utf8((const char *)str + offset,
		                           len - offset, cp);

		if (unlikely(len == SIZE_MAX && cp == 0)) {
			return 0;
		} else {
			return ret;
		}
	} else {
		*cp = GRAPHEME_INVALID_CODEPOINT;
		return 0;
	}
}

inline size_t
set_codepoint(uint_least32_t cp, void *str, size_t len, size_t offset)
{
	if (str == NULL || len == 0) {
		return 1;
	}

	if (offset < len) {
		((uint_least32_t *)str)[offset] = cp;
		return 1;
	} else {
		return 0;
	}
}

inline size_t
set_codepoint_utf8(uint_least32_t cp, void *str, size_t len, size_t offset)
{
	if (str == NULL || len == 0) {
		return grapheme_encode_utf8(cp, NULL, 0);
	}

	if (offset < len) {
		return grapheme_encode_utf8(cp, (char *)str + offset,
		                            len - offset);
	} else {
		return grapheme_encode_utf8(cp, NULL, 0);
	}
}
