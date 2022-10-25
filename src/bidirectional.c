/* See LICENSE file for copyright and license details. */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "../gen/bidirectional.h"
#include "../grapheme.h"
#include "util.h"

#define MAX_DEPTH 125

struct isolate_runner {
	int_least32_t *buf;
	size_t buflen;
	enum bidi_property prev_prop;
	struct {
		size_t off;
		enum bidi_property prop;
		int_least8_t level;
	} cur;
	struct {
		size_t off;
		enum bidi_property prop;
	} next;
	int_least8_t paragraph_level;
	int_least8_t isolating_run_level;
	enum bidi_property last_strong_type;
};

/*
 * we want to store the bidirectional property, the embedding level
 * and the visited-state (for sequential processing of rule X10)
 * all in a signed 16-bit-integer (because that's what we will write
 * the final embedding level into). To remain outside of implementation
 * defined territory, we can only effectively use 15 bits for bitwise
 * magic.
 *
 * Storage is still feasible, though, because the values all have very
 * limited ranges and can be stored as unsigned integers:
 *
 *   level+1 is in 0..MAX_DEPTH+1=126 which lies in 0..127 = 2^7-1
 *   visited is in 0..1               which lies in 0..1   = 2^1-1
 *   prop    is in 0..23              which lies in 0..31  = 2^5-1
 *   rawprop is in 0..23              which lies in 0..31  = 2^5-1
 *
 * yielding a total storage size of 18 bits.
 *
 * TODO: 7 bits for 0..127 bracket types, 1 bit for resolved paragraph
 *
 *       grapheme_bidirectional_preprocess(src, srclen, override, data, datalen)
 *       grapheme_get_bidirectional_line_embedding_levels(data, datalen, lev, levlen)
 *
 *       grapheme_get_bidirectional_line_levels
 *       grapheme_bidirectional_reorder_line
 */
static inline int_least8_t
buffy_get_level(int_least32_t val)
{
	return ((int_least8_t)(((uint_least32_t)val) & 0x7F /* 01111111 */)) - 1;
}

static inline void
buffy_set_level(int_least32_t *val, int_least8_t level)
{
	*val = (((uint_least32_t)(*val)) & UINT32_C(0x3FF80) /* 00000011.11111111.10000000 */) |
	       ((uint_least8_t)(level + 1) & 0x7F /* 01111111 */);
}

static inline bool
buffy_is_visited(int_least32_t val)
{
	return (((uint_least32_t)val) & 0x80); /* 10000000 */
}

static inline void
buffy_set_visited(int_least32_t *val, bool visited)
{
	*val = (int_least32_t)(
	       (((uint_least32_t)(*val)) & UINT32_C(0x3FF7F) /* 00000011.11111111.01111111 */) |
	       (((uint_least32_t)(visited)) << 7));
}

static inline enum bidi_property
buffy_get_prop(int_least32_t val)
{
	return (((uint_least32_t)val) & UINT32_C(0x1F00) /* 00011111.00000000 */) >> 8;
}

static inline void
buffy_set_prop(int_least32_t *val, enum bidi_property prop)
{
	*val = (int_least32_t)(
	       (((uint_least32_t)(*val)) & UINT32_C(0x3E0FF) /* 00000011.11100000.11111111 */) |
	       (((uint_least32_t)((uint_least8_t)(prop)) & 0x1F /* 00011111 */) << 8));
}

static inline enum bidi_property
buffy_get_rawprop(int_least32_t val)
{
	return (((uint_least32_t)val) & UINT32_C(0x3E000) /* 00000011.11100000.00000000 */) >> 13;
}

static inline void
buffy_set_rawprop(int_least32_t *val, enum bidi_property rawprop)
{
	*val = (int_least32_t)(
	       (((uint_least32_t)(*val)) & UINT32_C(0x1FFF) /* 00011111.11111111 */) |
	       (((uint_least32_t)((uint_least8_t)(rawprop)) & 0x1F /* 00011111 */) << 13));
}

static void
isolate_runner_init(int_least32_t *buf, size_t buflen, size_t off,
                    int_least8_t paragraph_level, bool within,
                    struct isolate_runner *ir)
{
	ssize_t i;
	int_least8_t cur_level, sos_level, level;

	/* initialize invariants */
	ir->buf = buf;
	ir->buflen = buflen;
	ir->paragraph_level = paragraph_level;
	ir->isolating_run_level = buffy_get_level(buf[off]);

	/* advance off until we are at a non-removed character */
	while (buffy_get_level(buf[off]) == -1) {
		off++;
	}

	/*
	 * we store the current offset in the next offset, so it is
	 * shifted in properly at the first advancement
	 */
	ir->next.off = off;
	ir->next.prop = buffy_get_prop(buf[off]);

	/*
	 * determine the previous state but store it in cur.prop
	 * cur.off is set to SIZE_MAX and cur.level to -1, as both are
	 * discarded on the first advancement anyway
	 */
	cur_level = buffy_get_level(buf[off]);
	ir->cur.prop = NUM_BIDI_PROPS;
	for (i = (ssize_t)off - 1, sos_level = -1; i >= 0; i--) {
		level = buffy_get_level(buf[i]);

		if (level != -1) {
			/*
			 * we found a character that has not been
			 * removed in X9
			 */
			sos_level = level;

			if (within) {
				/* we just take it */
				ir->cur.prop = buffy_get_prop(buf[i]);
			}

			break;
		}
	}
	if (sos_level == -1) {
		/*
		 * there were no preceding characters, set sos-level
		 * to paragraph embedding level
		 */
		sos_level = paragraph_level;
	}

	if (!within || ir->cur.prop == NUM_BIDI_PROPS) {
		/*
		 * we are at the beginning of the sequence; initialize
		 * it faithfully according to the algorithm by looking
		 * at the sos-level
		 */
		if (MAX(sos_level, cur_level) % 2 == 0) {
			/* the higher level is even, set sos to L */
			ir->cur.prop = BIDI_PROP_L;
		} else {
			/* the higher level is odd, set sos to R */
			ir->cur.prop = BIDI_PROP_R;
		}
	}

	ir->cur.off = SIZE_MAX;
	ir->cur.level = -1;
}

static int
isolate_runner_advance(struct isolate_runner *ir)
{
	enum bidi_property prop;
	int_least8_t level, isolate_level, last_isolate_level;
	size_t i;

	if (ir->next.off == SIZE_MAX) {
		/* the sequence is over */
		return 1;
	}

	/* shift in */
	ir->prev_prop = ir->cur.prop;
	ir->cur.off = ir->next.off;
	ir->cur.prop = ir->next.prop;
	ir->cur.level = buffy_get_level(ir->buf[ir->cur.off]);

	/* mark as visited */
	buffy_set_visited(&(ir->buf[ir->cur.off]), true);

	/*
	 * update last strong type, which is guaranteed to work properly
	 * on the first advancement as the prev_prop holds the sos type,
	 * which can only be either R or L, which are both strong types
	 */
	if (ir->prev_prop == BIDI_PROP_R ||
	    ir->prev_prop == BIDI_PROP_L ||
	    ir->prev_prop == BIDI_PROP_AL) {
		ir->last_strong_type = ir->prev_prop;
	}

	/* initialize next state by going to the next character in the sequence */
	ir->next.off = SIZE_MAX;
	ir->next.prop = NUM_BIDI_PROPS;

	last_isolate_level = -1;
	for (i = ir->cur.off, isolate_level = 0; i < ir->buflen; i++) {
		prop = buffy_get_prop(ir->buf[i]);
		level = buffy_get_level(ir->buf[i]);

		if (level == -1) {
			/* this is one of the ignored characters, skip */
			continue;
		} else if (level == ir->isolating_run_level) {
			last_isolate_level = level;
		}

		/* follow BD8/BD9 and P2 to traverse the current sequence */
		if (prop == BIDI_PROP_LRI ||
		    prop == BIDI_PROP_RLI ||
		    prop == BIDI_PROP_FSI) {
			/*
			 * we encountered an isolate initiator, increment
			 * counter, but go into processing when we
			 * were not isolated before
			 */
			if (isolate_level < MAX_DEPTH) {
				isolate_level++;
			}
			if (isolate_level != 1) {
				continue;
			}
		} else if (prop == BIDI_PROP_PDI &&
		           isolate_level > 0) {
			isolate_level--;

			/*
			 * if the current PDI dropped the isolate-level
			 * to zero, it is itself part of the isolating
			 * run sequence; otherwise we simply continue.
			 */
			if (isolate_level > 0) {
				continue;
			}
		} else if (isolate_level > 0) {
			/* we are in an isolating sequence */
			continue;
		}

		/*
		 * now we either still are in our sequence or we hit
		 * the eos-case as we left the sequence and hit the
		 * first non-isolating-sequence character.
		 */
		if (i == ir->cur.off) {
			/* we were in the first initializing round */
			continue;
		} else if (level == ir->isolating_run_level) {
			/* isolate_level-skips have been handled before, we're good */
			/* still in the sequence */
			ir->next.off = (size_t)i;
			ir->next.prop = buffy_get_prop(ir->buf[i]);
		} else {
			/* out of sequence or isolated, compare levels via eos */
			if (MAX(last_isolate_level, level) % 2 == 0) {
				ir->next.prop = BIDI_PROP_L;
			} else {
				ir->next.prop = BIDI_PROP_R;
			}
			ir->next.off = SIZE_MAX;
		}
		break;
	}
	if (i == ir->buflen) {
		/*
		 * the sequence ended before we could grab an offset.
		 * we need to determine the eos-prop by comparing the
		 * level of the last element in the isolating run sequence
		 * with the paragraph level.
		 */
		if (MAX(last_isolate_level, ir->paragraph_level) % 2 == 0) {
			/* the higher level is even, set eos to L */
			ir->next.prop = BIDI_PROP_L;
		} else {
			/* the higher level is odd, set sos to R */
			ir->next.prop = BIDI_PROP_R;
		}
		ir->next.off = SIZE_MAX;
	}

	return 0;
}

static void
isolate_runner_set_current_prop(struct isolate_runner *ir, enum bidi_property prop)
{
	buffy_set_prop(&(ir->buf[ir->cur.off]), prop);
	ir->cur.prop = prop;
}

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

static size_t
process_isolating_run_sequence(int_least32_t *buf, size_t buflen,
                               size_t off, int_least8_t paragraph_level)
{
	enum bidi_property sequence_prop;
	struct isolate_runner ir, tmp;
	size_t runsince, sequence_end;

	/* W1 */
	isolate_runner_init(buf, buflen, off, paragraph_level, false, &ir);
	while (!isolate_runner_advance(&ir)) {
		if (ir.cur.prop == BIDI_PROP_NSM) {
			if (ir.prev_prop == BIDI_PROP_LRI ||
			    ir.prev_prop == BIDI_PROP_RLI ||
			    ir.prev_prop == BIDI_PROP_FSI ||
			    ir.prev_prop == BIDI_PROP_PDI) {
				isolate_runner_set_current_prop(&ir, BIDI_PROP_ON);
			} else {
				isolate_runner_set_current_prop(&ir,
				                                ir.prev_prop);
			}
		}
	}

	/* W2 */
	isolate_runner_init(buf, buflen, off, paragraph_level, false, &ir);
	while (!isolate_runner_advance(&ir)) {
		if (ir.cur.prop == BIDI_PROP_EN &&
		    ir.last_strong_type == BIDI_PROP_AL) {
			isolate_runner_set_current_prop(&ir, BIDI_PROP_AN);
		}
	}

	/* W3 */
	isolate_runner_init(buf, buflen, off, paragraph_level, false, &ir);
	while (!isolate_runner_advance(&ir)) {
		if (ir.cur.prop == BIDI_PROP_AL) {
			isolate_runner_set_current_prop(&ir, BIDI_PROP_R);
		}
	}

	/* W4 */
	isolate_runner_init(buf, buflen, off, paragraph_level, false, &ir);
	while (!isolate_runner_advance(&ir)) {
		if (ir.prev_prop == BIDI_PROP_EN &&
		    (ir.cur.prop == BIDI_PROP_ES ||
		     ir.cur.prop == BIDI_PROP_CS) &&
		    ir.next.prop == BIDI_PROP_EN) {
			isolate_runner_set_current_prop(&ir, BIDI_PROP_EN);
		}

		if (ir.prev_prop == BIDI_PROP_AN &&
		    ir.cur.prop  == BIDI_PROP_CS &&
		    ir.next.prop == BIDI_PROP_AN) {
			isolate_runner_set_current_prop(&ir, BIDI_PROP_AN);
		}
	}

	/* W5 */
	runsince = SIZE_MAX;
	isolate_runner_init(buf, buflen, off, paragraph_level, false, &ir);
	while (!isolate_runner_advance(&ir)) {
		if (ir.cur.prop == BIDI_PROP_ET) {
			if (runsince == SIZE_MAX) {
				/* a new run has begun */
				runsince = ir.cur.off;
			}
		} else if (ir.cur.prop == BIDI_PROP_EN) {
			/* set the preceding sequence */
			if (runsince != SIZE_MAX) {
				isolate_runner_init(buf, buflen, runsince, paragraph_level, (runsince > off), &tmp);
				while (!isolate_runner_advance(&tmp) &&
				       tmp.cur.off < ir.cur.off) {
					isolate_runner_set_current_prop(&tmp, BIDI_PROP_EN);
				}
				runsince = SIZE_MAX;
			} else {
				isolate_runner_init(buf, buflen, ir.cur.off, paragraph_level, (ir.cur.off > off), &tmp);
				isolate_runner_advance(&tmp);
			}
			/* follow the succeeding sequence */
			while (!isolate_runner_advance(&tmp)) {
				if (tmp.cur.prop != BIDI_PROP_ET) {
					break;
				}
				isolate_runner_set_current_prop(&tmp, BIDI_PROP_EN);
			}
		} else {
			/* sequence ended */
			runsince = SIZE_MAX;
		}
	}

	/* W6 */
	isolate_runner_init(buf, buflen, off, paragraph_level, false, &ir);
	while (!isolate_runner_advance(&ir)) {
		if (ir.cur.prop == BIDI_PROP_ES ||
		    ir.cur.prop == BIDI_PROP_ET ||
		    ir.cur.prop == BIDI_PROP_CS) {
			isolate_runner_set_current_prop(&ir, BIDI_PROP_ON);
		}
	}

	/* W7 */
	isolate_runner_init(buf, buflen, off, paragraph_level, false, &ir);
	while (!isolate_runner_advance(&ir)) {
		if (ir.cur.prop == BIDI_PROP_EN &&
		    ir.last_strong_type == BIDI_PROP_L) {
			isolate_runner_set_current_prop(&ir, BIDI_PROP_L);
		}
	}

	/* N0 */

	/* N1 */
	sequence_end = SIZE_MAX;
	sequence_prop = NUM_BIDI_PROPS;
	isolate_runner_init(buf, buflen, off, paragraph_level, false, &ir);
	while (!isolate_runner_advance(&ir)) {
		if (sequence_end == SIZE_MAX) {
			if (ir.cur.prop == BIDI_PROP_B   ||
			    ir.cur.prop == BIDI_PROP_S   ||
			    ir.cur.prop == BIDI_PROP_WS  ||
			    ir.cur.prop == BIDI_PROP_ON  ||
			    ir.cur.prop == BIDI_PROP_FSI ||
			    ir.cur.prop == BIDI_PROP_LRI ||
			    ir.cur.prop == BIDI_PROP_RLI ||
			    ir.cur.prop == BIDI_PROP_PDI) {
				/* the current character is an NI (neutral or isolate) */

				/* scan ahead to the end of the NI-sequence */
				isolate_runner_init(buf, buflen, ir.cur.off, paragraph_level, (ir.cur.off > off), &tmp);
				while (!isolate_runner_advance(&tmp)) {
					if (tmp.next.prop != BIDI_PROP_B   &&
					    tmp.next.prop != BIDI_PROP_S   &&
					    tmp.next.prop != BIDI_PROP_WS  &&
					    tmp.next.prop != BIDI_PROP_ON  &&
					    tmp.next.prop != BIDI_PROP_FSI &&
					    tmp.next.prop != BIDI_PROP_LRI &&
					    tmp.next.prop != BIDI_PROP_RLI &&
					    tmp.next.prop != BIDI_PROP_PDI) {
						break;
					}
				}

				/*
				 * check what follows and see if the text has the
				 * same direction on both sides
				 */
				if (ir.prev_prop == BIDI_PROP_L &&
				    tmp.next.prop == BIDI_PROP_L) {
					sequence_end = tmp.cur.off;
					sequence_prop = BIDI_PROP_L;
				} else if ((ir.prev_prop == BIDI_PROP_R  ||
				            ir.prev_prop == BIDI_PROP_EN ||
				            ir.prev_prop == BIDI_PROP_AN) &&
				           (tmp.next.prop == BIDI_PROP_R  ||
				            tmp.next.prop == BIDI_PROP_EN ||
				            tmp.next.prop == BIDI_PROP_AN)) {
					sequence_end = tmp.cur.off;
					sequence_prop = BIDI_PROP_R;
				}
			}
		}

		if (sequence_end != SIZE_MAX) {
			if (ir.cur.off <= sequence_end) {
				isolate_runner_set_current_prop(&ir, sequence_prop);
			} else {
				/* end of sequence, reset */
				sequence_end = SIZE_MAX;
				sequence_prop = NUM_BIDI_PROPS;
			}
		}
	}

	/* N2 */
	isolate_runner_init(buf, buflen, off, paragraph_level, false, &ir);
	while (!isolate_runner_advance(&ir)) {
		if (ir.cur.prop == BIDI_PROP_B   ||
		    ir.cur.prop == BIDI_PROP_S   ||
		    ir.cur.prop == BIDI_PROP_WS  ||
		    ir.cur.prop == BIDI_PROP_ON  ||
		    ir.cur.prop == BIDI_PROP_FSI ||
		    ir.cur.prop == BIDI_PROP_LRI ||
		    ir.cur.prop == BIDI_PROP_RLI ||
		    ir.cur.prop == BIDI_PROP_PDI) {
			/* N2 */
			if (ir.cur.level % 2 == 0) {
				/* even embedding level */
				isolate_runner_set_current_prop(&ir, BIDI_PROP_L);
			} else {
				/* odd embedding level */
				isolate_runner_set_current_prop(&ir, BIDI_PROP_R);
			}
		}
	}

	return 0;
}

static int_least8_t
get_paragraph_level(enum grapheme_bidirectional_override override,
                    bool terminate_on_pdi,
                    const int_least32_t *buf, size_t buflen)
{
	enum bidi_property prop;
	int_least8_t isolate_level;
	size_t bufoff;

	/* check overrides first according to rule HL1 */
	if (override == GRAPHEME_BIDIRECTIONAL_OVERRIDE_LTR) {
		return 0;
	} else if (override == GRAPHEME_BIDIRECTIONAL_OVERRIDE_RTL) {
		return 1;
	}

	/* determine paragraph level (rules P1-P3) */

	for (bufoff = 0, isolate_level = 0; bufoff < buflen; bufoff++) {
		prop = buffy_get_prop(buf[bufoff]);

		if (prop == BIDI_PROP_PDI &&
		    isolate_level == 0 &&
		    terminate_on_pdi) {
			/*
			 * we are in a FSI-subsection of a paragraph and
			 * matched with the terminating PDI
			 */
			break;
		}

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
get_paragraph_embedding_levels(enum grapheme_bidirectional_override override,
                               int_least32_t *buf, size_t buflen)
{
	struct {
		int_least8_t level;
		enum grapheme_bidirectional_override override;
		bool directional_isolate;
	} directional_status[MAX_DEPTH + 2], *dirstat = directional_status;
	enum bidi_property prop, rawprop;
	size_t overflow_isolate_count, overflow_embedding_count,
	       valid_isolate_count, bufoff, i, runsince;
	int_least8_t paragraph_level, level;

	paragraph_level = get_paragraph_level(override, false, buf, buflen);

	/* X1 */
	dirstat->level = paragraph_level;
	dirstat->override = GRAPHEME_BIDIRECTIONAL_OVERRIDE_NEUTRAL;
	dirstat->directional_isolate = false;
	overflow_isolate_count = overflow_embedding_count = valid_isolate_count = 0;

	for (bufoff = 0; bufoff < buflen; bufoff++) {
		prop = buffy_get_prop(buf[bufoff]);
again:
		if (prop == BIDI_PROP_RLE) {
			/* X2 */
			if (dirstat->level + (dirstat->level % 2 != 0) + 1 <= MAX_DEPTH &&
			    overflow_isolate_count == 0 &&
			    overflow_embedding_count == 0) {
				/* valid RLE */
				dirstat++;
				dirstat->level = (dirstat - 1)->level + ((dirstat - 1)->level % 2 != 0) + 1;
				dirstat->override = GRAPHEME_BIDIRECTIONAL_OVERRIDE_NEUTRAL;
				dirstat->directional_isolate = false;
			} else {
				/* overflow RLE */
				overflow_embedding_count += (overflow_isolate_count == 0);
			}
		} else if (prop == BIDI_PROP_LRE) {
			/* X3 */
			if (dirstat->level + (dirstat->level % 2 == 0) + 1 <= MAX_DEPTH &&
			    overflow_isolate_count == 0 &&
			    overflow_embedding_count == 0) {
				/* valid LRE */
				dirstat++;
				dirstat->level = (dirstat - 1)->level + ((dirstat - 1)->level % 2 == 0) + 1;
				dirstat->override = GRAPHEME_BIDIRECTIONAL_OVERRIDE_NEUTRAL;
				dirstat->directional_isolate = false;
			} else {
				/* overflow LRE */
				overflow_embedding_count += (overflow_isolate_count == 0);
			}
		} else if (prop == BIDI_PROP_RLO) {
			/* X4 */
			if (dirstat->level + (dirstat->level % 2 != 0) + 1 <= MAX_DEPTH &&
			    overflow_isolate_count == 0 &&
			    overflow_embedding_count == 0) {
				/* valid RLO */
				dirstat++;
				dirstat->level = (dirstat - 1)->level + ((dirstat - 1)->level % 2 != 0) + 1;
				dirstat->override = GRAPHEME_BIDIRECTIONAL_OVERRIDE_RTL;
				dirstat->directional_isolate = false;
			} else {
				/* overflow RLO */
				overflow_embedding_count += (overflow_isolate_count == 0);
			}
		} else if (prop == BIDI_PROP_LRO) {
			/* X5 */
			if (dirstat->level + (dirstat->level % 2 == 0) + 1 <= MAX_DEPTH &&
			    overflow_isolate_count == 0 &&
			    overflow_embedding_count == 0) {
				/* valid LRE */
				dirstat++;
				dirstat->level = (dirstat - 1)->level + ((dirstat - 1)->level % 2 == 0) + 1;
				dirstat->override = GRAPHEME_BIDIRECTIONAL_OVERRIDE_LTR;
				dirstat->directional_isolate = false;
			} else {
				/* overflow LRO */
				overflow_embedding_count += (overflow_isolate_count == 0);
			}
		} else if (prop == BIDI_PROP_RLI) {
			/* X5a */
			buffy_set_level(&(buf[bufoff]), dirstat->level);

			if (dirstat->override == GRAPHEME_BIDIRECTIONAL_OVERRIDE_LTR) {
				buffy_set_prop(&(buf[bufoff]), BIDI_PROP_L);
			} else if (dirstat->override == GRAPHEME_BIDIRECTIONAL_OVERRIDE_RTL) {
				buffy_set_prop(&(buf[bufoff]), BIDI_PROP_R);
			}

			if (dirstat->level + (dirstat->level % 2 != 0) + 1 <= MAX_DEPTH &&
			    overflow_isolate_count == 0 &&
			    overflow_embedding_count == 0) {
				/* valid RLI */
				valid_isolate_count++;

				dirstat++;
				dirstat->level = (dirstat - 1)->level + ((dirstat - 1)->level % 2 != 0) + 1;
				dirstat->override = GRAPHEME_BIDIRECTIONAL_OVERRIDE_NEUTRAL;
				dirstat->directional_isolate = true;
			} else {
				/* overflow RLI */
				overflow_isolate_count++;
			}
		} else if (prop == BIDI_PROP_LRI) {
			/* X5b */
			buffy_set_level(&(buf[bufoff]), dirstat->level);

			if (dirstat->override == GRAPHEME_BIDIRECTIONAL_OVERRIDE_LTR) {
				buffy_set_prop(&(buf[bufoff]), BIDI_PROP_L);
			} else if (dirstat->override == GRAPHEME_BIDIRECTIONAL_OVERRIDE_RTL) {
				buffy_set_prop(&(buf[bufoff]), BIDI_PROP_R);
			}

			if (dirstat->level + (dirstat->level % 2 == 0) + 1 <= MAX_DEPTH &&
			    overflow_isolate_count == 0 &&
			    overflow_embedding_count == 0) {
				/* valid LRI */
				valid_isolate_count++;

				dirstat++;
				dirstat->level = (dirstat - 1)->level + ((dirstat - 1)->level % 2 == 0) + 1;
				dirstat->override = GRAPHEME_BIDIRECTIONAL_OVERRIDE_NEUTRAL;
				dirstat->directional_isolate = true;
			} else {
				/* overflow LRI */
				overflow_isolate_count++;
			}
		} else if (prop == BIDI_PROP_FSI) {
			/* X5c */
			if (get_paragraph_level(GRAPHEME_BIDIRECTIONAL_OVERRIDE_NEUTRAL, true,
			                        buf + (bufoff + 1), buflen - (bufoff + 1)) == 1) {
				prop = BIDI_PROP_RLI;
				goto again;
			} else { /* ... == 0 */
				prop = BIDI_PROP_LRI;
				goto again;
			}
		} else if (prop != BIDI_PROP_B   &&
		           prop != BIDI_PROP_BN  &&
		           prop != BIDI_PROP_PDF &&
		           prop != BIDI_PROP_PDI) {
			/* X6 */
			buffy_set_level(&(buf[bufoff]), dirstat->level);

			if (dirstat->override == GRAPHEME_BIDIRECTIONAL_OVERRIDE_LTR) {
				buffy_set_prop(&(buf[bufoff]), BIDI_PROP_L);
			} else if (dirstat->override == GRAPHEME_BIDIRECTIONAL_OVERRIDE_RTL) {
				buffy_set_prop(&(buf[bufoff]), BIDI_PROP_R);
			}
		} else if (prop == BIDI_PROP_PDI) {
			/* X6a */
			if (overflow_isolate_count > 0) {
				/* PDI matches an overflow isolate initiator */
				overflow_isolate_count--;
			} else if (valid_isolate_count > 0) {
				/* PDI matches a normal isolate initiator */
				overflow_embedding_count = 0;
				while (dirstat->directional_isolate == false &&
				       dirstat > directional_status) {
					/*
					 * we are safe here as given the
					 * valid isolate count is positive
					 * there must be a stack-entry
					 * with positive directional
					 * isolate status, but we take
					 * no chances and include an
					 * explicit check
					 *
					 * POSSIBLE OPTIMIZATION: Whenever
					 * we push on the stack, check if it
					 * has the directional isolate status
					 * true and store a pointer to it
					 * so we can jump to it very quickly.
					 */
					dirstat--;
				}

				/*
				 * as above, the following check is not
				 * necessary, given we are guaranteed to
				 * have at least one stack entry left,
				 * but it's better to be safe
				 */
				if (dirstat > directional_status) {
					dirstat--;
				}
				valid_isolate_count--;
			}

			buffy_set_level(&(buf[bufoff]), dirstat->level);

			if (dirstat->override == GRAPHEME_BIDIRECTIONAL_OVERRIDE_LTR) {
				buffy_set_prop(&(buf[bufoff]), BIDI_PROP_L);
			} else if (dirstat->override == GRAPHEME_BIDIRECTIONAL_OVERRIDE_RTL) {
				buffy_set_prop(&(buf[bufoff]), BIDI_PROP_R);
			}
		} else if (prop == BIDI_PROP_PDF) {
			/* X7 */
			if (overflow_isolate_count > 0) {
				/* do nothing */
			} else if (overflow_embedding_count > 0) {
				overflow_embedding_count--;
			} else if (dirstat->directional_isolate == false &&
			           dirstat > directional_status) {
				dirstat--;
			}
		} else if (prop == BIDI_PROP_B) {
			/* X8 */
			buffy_set_level(&(buf[bufoff]), paragraph_level);
		}

		/* X9 */
		if (prop == BIDI_PROP_RLE ||
		    prop == BIDI_PROP_LRE ||
		    prop == BIDI_PROP_RLO ||
		    prop == BIDI_PROP_LRO ||
		    prop == BIDI_PROP_PDF ||
		    prop == BIDI_PROP_BN) {
			buffy_set_level(&(buf[bufoff]), -1);
		}
	}

	/* X10 (W1-W7, N0-N2) */
	for (bufoff = 0; bufoff < buflen; bufoff++) {
		if (!buffy_is_visited(buf[bufoff]) && buffy_get_level(buf[bufoff]) != -1) {
			bufoff += process_isolating_run_sequence(buf, buflen, bufoff,
			                                         paragraph_level);
		}
	}

	/*
	 * I1-I2 (given our sequential approach to processing the
	 * isolating run sequences, we apply this rule separately)
	 */
	for (bufoff = 0; bufoff < buflen; bufoff++) {
		prop = buffy_get_prop(buf[bufoff]);
		level = buffy_get_level(buf[bufoff]);

		if (level % 2 == 0 ) {
			/* even level */
			if (prop == BIDI_PROP_R) {
				level += 1;
			} else if (prop == BIDI_PROP_AN ||
			           prop == BIDI_PROP_EN) {
				level += 2;
			}
		} else {
			/* odd level */
			if (prop == BIDI_PROP_L  ||
			    prop == BIDI_PROP_EN ||
			    prop == BIDI_PROP_AN) {
				level += 1;
			}
		}

		buffy_set_level(&(buf[bufoff]), level);
	}

	/* L1 (rules 1-3) */
	runsince = SIZE_MAX;
	for (bufoff = 0; bufoff < buflen; bufoff++) {
		rawprop = buffy_get_rawprop(buf[bufoff]);
		level = buffy_get_level(buf[bufoff]);

		if (level == -1) {
			/* ignored character */
			continue;
		}

		if (rawprop == BIDI_PROP_WS  ||
		    rawprop == BIDI_PROP_FSI ||
		    rawprop == BIDI_PROP_LRI ||
		    rawprop == BIDI_PROP_RLI ||
		    rawprop == BIDI_PROP_PDI) {
			if (runsince == SIZE_MAX) {
				/* a new run has begun */
				runsince = bufoff;
			}
		} else if (rawprop == BIDI_PROP_S ||
		           rawprop == BIDI_PROP_B) {
			/* L1.4 -- ignored for now, < beachten! */
			for (i = runsince; i < bufoff; i++) {
				if (buffy_get_level(buf[i]) != -1) {
					buffy_set_level(&(buf[i]), paragraph_level);
				}
			}
			runsince = SIZE_MAX;
		} else {
			/* sequence ended */
			runsince = SIZE_MAX;
		}

		if (rawprop == BIDI_PROP_S ||
		    rawprop == BIDI_PROP_B) {
			buffy_set_level(&(buf[bufoff]), paragraph_level);
		}
		continue;
	}
	if (runsince != SIZE_MAX) {
		/*
		 * this is the end of the line and we
		 * are in a run
		 */
		for (i = runsince; i < buflen; i++) {
			if (buffy_get_level(buf[i]) != -1) {
				buffy_set_level(&(buf[i]), paragraph_level);
			}
		}
		runsince = SIZE_MAX;
	}
}

static size_t
get_embedding_levels(HERODOTUS_READER *r, enum grapheme_bidirectional_override override,
                     int_least32_t *buf, size_t buflen)
{
	size_t bufoff, bufsize, lastparoff;
	uint_least32_t cp;

	if (buf == NULL) {
		for (; herodotus_read_codepoint(r, true, &cp) ==
		     HERODOTUS_STATUS_SUCCESS;)
			;

		/* see below for return value reasoning */
		return herodotus_reader_number_read(r);
	}

	/*
	 * the first step is to determine the bidirectional properties
	 * and store them in the buffer
	 */
	for (bufoff = 0; herodotus_read_codepoint(r, true, &cp) ==
	     HERODOTUS_STATUS_SUCCESS; bufoff++) {
		if (bufoff < buflen) {
			/*
			 * actually only do something when we have
			 * space in the level-buffer. We continue
			 * the iteration to be able to give a good
			 * return value
			 */
			buf[bufoff] = 0;
			buffy_set_prop(&(buf[bufoff]), get_bidi_property(cp));
			buffy_set_rawprop(&(buf[bufoff]), get_bidi_property(cp));
		}
	}
	bufsize = herodotus_reader_number_read(r);

	for (bufoff = 0, lastparoff = 0; bufoff < bufsize; bufoff++) {
		if (buffy_get_prop(buf[bufoff]) != BIDI_PROP_B &&
		    bufoff != bufsize - 1) {
			continue;
		}

		/*
		 * we either encountered a paragraph terminator or this
		 * is the last character in the string.
		 * Call the paragraph handler on the paragraph, including
		 * the terminating character or last character of the
		 * string respectively
		 */
		get_paragraph_embedding_levels(override, buf + lastparoff,
		                               bufoff + 1 - lastparoff);
		lastparoff = bufoff + 1;
	}

	/* bake the levels into the buffer, discarding the metadata */
	for (bufoff = 0; bufoff < bufsize; bufoff++) {
		buf[bufoff] = buffy_get_level(buf[bufoff]);
	}

	/*
	 * we return the number of total bytes read, as the function
	 * should indicate if the given level-buffer is too small
	 */
	return herodotus_reader_number_read(r);
}

size_t
grapheme_get_bidirectional_embedding_levels(const uint_least32_t *src, size_t srclen,
                                            enum grapheme_bidirectional_override override,
                                            int_least32_t *dest, size_t destlen)
{
	HERODOTUS_READER r;

	herodotus_reader_init(&r, HERODOTUS_TYPE_CODEPOINT, src, srclen);

	return get_embedding_levels(&r, override, dest, destlen);
}

size_t
grapheme_get_bidirectional_embedding_levels_utf8(const char *src, size_t srclen,
                                                 enum grapheme_bidirectional_override override,
                                                 int_least32_t *dest, size_t destlen)
{
	HERODOTUS_READER r;

	herodotus_reader_init(&r, HERODOTUS_TYPE_UTF8, src, srclen);

	return get_embedding_levels(&r, override, dest, destlen);
}
