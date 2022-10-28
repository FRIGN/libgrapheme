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
	uint_least8_t paragraph_level;
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
 *   paragraph_level is in 0..1               which lies in 0..1   = 2^1-1
 *   level+1         is in 0..MAX_DEPTH+1=126 which lies in 0..127 = 2^7-1
 *   prop            is in 0..23              which lies in 0..31  = 2^5-1
 *   bracket_off     is in 0..255             which lies in 0..255 = 2^8-1
 *   visited         is in 0..1               which lies in 0..1   = 2^1-1
 *
 * yielding a total storage size of 22 bits.
 */
struct state {
	uint_least8_t paragraph_level;
	int_least8_t level;
	enum bidi_property prop;
	uint_least8_t bracket_off;
	bool visited;
	enum bidi_property rawprop;
};

static inline void
state_serialize(const struct state *s, int_least32_t *out)
{
	*out = (int_least32_t)(
	       ((((uint_least32_t)(s->paragraph_level)) & 0x01 /* 00000001 */) <<  0) |
	       ((((uint_least32_t)(s->level + 1))       & 0x7F /* 01111111 */) <<  1) |
	       ((((uint_least32_t)(s->prop))            & 0x1F /* 00011111 */) <<  8) |
	       ((((uint_least32_t)(s->bracket_off))     & 0xFF /* 11111111 */) << 13) |
	       ((((uint_least32_t)(s->visited))         & 0x01 /* 00000001 */) << 21) |
	       ((((uint_least32_t)(s->rawprop))         & 0x1F /* 00011111 */) << 22));
}

static inline void
state_deserialize(int_least32_t in, struct state *s)
{
	s->paragraph_level =      (uint_least8_t)((((uint_least32_t)in) >>  0) & 0x01 /* 00000001 */);
	s->level           =       (int_least8_t)((((uint_least32_t)in) >>  1) & 0x7F /* 01111111 */) - 1;
	s->prop            = (enum bidi_property)((((uint_least32_t)in) >>  8) & 0x1F /* 00011111 */);
	s->bracket_off     =      (uint_least8_t)((((uint_least32_t)in) >> 13) & 0xFF /* 11111111 */);
	s->visited         =               (bool)((((uint_least32_t)in) >> 21) & 0x01 /* 00000001 */);
	s->rawprop         = (enum bidi_property)((((uint_least32_t)in) >> 22) & 0x1F /* 00011111 */);
}

static void
isolate_runner_init(int_least32_t *buf, size_t buflen, size_t off,
                    uint_least8_t paragraph_level, bool within,
                    struct isolate_runner *ir)
{
	struct state s;
	ssize_t i;
	int_least8_t cur_level, sos_level;

	state_deserialize(buf[off], &s);

	/* initialize invariants */
	ir->buf = buf;
	ir->buflen = buflen;
	ir->paragraph_level = paragraph_level;
	ir->isolating_run_level = s.level;

	/* advance off until we are at a non-removed character */
	while (s.level == -1) {
		off++;
		state_deserialize(buf[off], &s);
	}

	/*
	 * we store the current offset in the next offset, so it is
	 * shifted in properly at the first advancement
	 */
	ir->next.off = off;
	ir->next.prop = s.prop;

	/*
	 * determine the previous state but store it in cur.prop
	 * cur.off is set to SIZE_MAX and cur.level to -1, as both are
	 * discarded on the first advancement anyway
	 */
	cur_level = s.level;
	ir->cur.prop = NUM_BIDI_PROPS;
	for (i = (ssize_t)off - 1, sos_level = -1; i >= 0; i--) {
		state_deserialize(buf[i], &s);

		if (s.level != -1) {
			/*
			 * we found a character that has not been
			 * removed in X9
			 */
			sos_level = s.level;

			if (within) {
				/* we just take it */
				ir->cur.prop = s.prop;
			}

			break;
		}
	}
	if (sos_level == -1) {
		/*
		 * there were no preceding characters, set sos-level
		 * to paragraph embedding level
		 */
		sos_level = (int_least8_t)paragraph_level;
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
	struct state s;
	int_least8_t isolate_level, last_isolate_level;
	size_t i;

	if (ir->next.off == SIZE_MAX) {
		/* the sequence is over */
		return 1;
	}


	/* shift in */
	ir->prev_prop = ir->cur.prop;
	ir->cur.off = ir->next.off;
	state_deserialize(ir->buf[ir->cur.off], &s);
	ir->cur.prop = ir->next.prop;
	ir->cur.level = s.level;

	/* mark as visited */
	s.visited = true;
	state_serialize(&s, &(ir->buf[ir->cur.off]));

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
		state_deserialize(ir->buf[i], &s);

		if (s.level == -1) {
			/* this is one of the ignored characters, skip */
			continue;
		} else if (s.level == ir->isolating_run_level) {
			last_isolate_level = s.level;
		}

		/* follow BD8/BD9 and P2 to traverse the current sequence */
		if (s.prop == BIDI_PROP_LRI ||
		    s.prop == BIDI_PROP_RLI ||
		    s.prop == BIDI_PROP_FSI) {
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
		} else if (s.prop == BIDI_PROP_PDI &&
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
		} else if (s.level == ir->isolating_run_level) {
			/* isolate_level-skips have been handled before, we're good */
			/* still in the sequence */
			ir->next.off = (size_t)i;
			ir->next.prop = s.prop;
		} else {
			/* out of sequence or isolated, compare levels via eos */
			if (MAX(last_isolate_level, s.level) % 2 == 0) {
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
	struct state s;

	state_deserialize(ir->buf[ir->cur.off], &s);
	s.prop = prop;
	state_serialize(&s, &(ir->buf[ir->cur.off]));

	ir->cur.prop = prop;
}

static inline enum bidi_property
get_bidi_property(uint_least32_t cp)
{
	if (likely(cp <= 0x10FFFF)) {
		return (enum bidi_property)
		       ((bidi_minor[bidi_major[cp >> 8] + (cp & 0xff)]) &
		        0x1F /* 00011111 */);
	} else {
		return BIDI_PROP_L;
	}
}

static inline uint_least8_t
get_bidi_bracket_off(uint_least32_t cp)
{
	if (likely(cp <= 0x10FFFF)) {
		return (bidi_minor[bidi_major[cp >> 8] + (cp & 0xff)]) >> 5;
	} else {
		return 0;
	}
}

static size_t
process_isolating_run_sequence(int_least32_t *buf, size_t buflen,
                               size_t off, uint_least8_t paragraph_level)
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

static uint_least8_t
get_paragraph_level(enum grapheme_bidirectional_override override,
                    bool terminate_on_pdi,
                    const int_least32_t *buf, size_t buflen)
{
	struct state s;
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
		state_deserialize(buf[bufoff], &s);

		if (s.prop == BIDI_PROP_PDI &&
		    isolate_level == 0 &&
		    terminate_on_pdi) {
			/*
			 * we are in a FSI-subsection of a paragraph and
			 * matched with the terminating PDI
			 */
			break;
		}

		/* BD8/BD9 */
		if ((s.prop == BIDI_PROP_LRI ||
		     s.prop == BIDI_PROP_RLI ||
		     s.prop == BIDI_PROP_FSI) &&
		    isolate_level < MAX_DEPTH) {
			/* we hit an isolate initiator, increment counter */
			isolate_level++;
		} else if (s.prop == BIDI_PROP_PDI && isolate_level > 0) {
			isolate_level--;
		}

		/* P2 */
		if (isolate_level > 0) {
			continue;
		}

		/* P3 */
		if (s.prop == BIDI_PROP_L) {
			return 0;
		} else if (s.prop == BIDI_PROP_AL ||
		           s.prop == BIDI_PROP_R) {
			return 1;
		}
	}

	return 0;
}

static void
get_paragraph_embedding_levels(enum grapheme_bidirectional_override override,
                               int_least32_t *buf, size_t buflen)
{
	enum bidi_property tmp_prop;
	struct state s, t;
	struct {
		int_least8_t level;
		enum grapheme_bidirectional_override override;
		bool directional_isolate;
	} directional_status[MAX_DEPTH + 2], *dirstat = directional_status;
	size_t overflow_isolate_count, overflow_embedding_count,
	       valid_isolate_count, bufoff, i, runsince;
	uint_least8_t paragraph_level;

	paragraph_level = get_paragraph_level(override, false, buf, buflen);

	/* X1 */
	dirstat->level = (int_least8_t)paragraph_level;
	dirstat->override = GRAPHEME_BIDIRECTIONAL_OVERRIDE_NEUTRAL;
	dirstat->directional_isolate = false;
	overflow_isolate_count = overflow_embedding_count = valid_isolate_count = 0;

	for (bufoff = 0; bufoff < buflen; bufoff++) {
		state_deserialize(buf[bufoff], &s);
		tmp_prop = s.prop;
again:
		if (tmp_prop == BIDI_PROP_RLE) {
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
		} else if (tmp_prop == BIDI_PROP_LRE) {
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
		} else if (tmp_prop == BIDI_PROP_RLO) {
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
		} else if (tmp_prop == BIDI_PROP_LRO) {
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
		} else if (tmp_prop == BIDI_PROP_RLI) {
			/* X5a */
			s.level = dirstat->level;
			if (dirstat->override == GRAPHEME_BIDIRECTIONAL_OVERRIDE_LTR) {
				s.prop = BIDI_PROP_L;
			} else if (dirstat->override == GRAPHEME_BIDIRECTIONAL_OVERRIDE_RTL) {
				s.prop = BIDI_PROP_R;
			}
			state_serialize(&s, &(buf[bufoff]));

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
		} else if (tmp_prop == BIDI_PROP_LRI) {
			/* X5b */
			s.level = dirstat->level;
			if (dirstat->override == GRAPHEME_BIDIRECTIONAL_OVERRIDE_LTR) {
				s.prop = BIDI_PROP_L;
			} else if (dirstat->override == GRAPHEME_BIDIRECTIONAL_OVERRIDE_RTL) {
				s.prop = BIDI_PROP_R;
			}
			state_serialize(&s, &(buf[bufoff]));

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
		} else if (tmp_prop == BIDI_PROP_FSI) {
			/* X5c */
			if (get_paragraph_level(GRAPHEME_BIDIRECTIONAL_OVERRIDE_NEUTRAL, true,
			                        buf + (bufoff + 1), buflen - (bufoff + 1)) == 1) {
				tmp_prop = BIDI_PROP_RLI;
				goto again;
			} else { /* ... == 0 */
				tmp_prop = BIDI_PROP_LRI;
				goto again;
			}
		} else if (tmp_prop != BIDI_PROP_B   &&
		           tmp_prop != BIDI_PROP_BN  &&
		           tmp_prop != BIDI_PROP_PDF &&
		           tmp_prop != BIDI_PROP_PDI) {
			/* X6 */
			s.level = dirstat->level;
			if (dirstat->override == GRAPHEME_BIDIRECTIONAL_OVERRIDE_LTR) {
				s.prop = BIDI_PROP_L;
			} else if (dirstat->override == GRAPHEME_BIDIRECTIONAL_OVERRIDE_RTL) {
				s.prop = BIDI_PROP_R;
			}
			state_serialize(&s, &(buf[bufoff]));
		} else if (tmp_prop == BIDI_PROP_PDI) {
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

			s.level = dirstat->level;
			if (dirstat->override == GRAPHEME_BIDIRECTIONAL_OVERRIDE_LTR) {
				s.prop = BIDI_PROP_L;
			} else if (dirstat->override == GRAPHEME_BIDIRECTIONAL_OVERRIDE_RTL) {
				s.prop = BIDI_PROP_R;
			}
			state_serialize(&s, &(buf[bufoff]));
		} else if (tmp_prop == BIDI_PROP_PDF) {
			/* X7 */
			if (overflow_isolate_count > 0) {
				/* do nothing */
			} else if (overflow_embedding_count > 0) {
				overflow_embedding_count--;
			} else if (dirstat->directional_isolate == false &&
			           dirstat > directional_status) {
				dirstat--;
			}
		} else if (tmp_prop == BIDI_PROP_B) {
			/* X8 */
			s.level = (int_least8_t)paragraph_level;
			state_serialize(&s, &(buf[bufoff]));
		}

		/* X9 */
		if (tmp_prop == BIDI_PROP_RLE ||
		    tmp_prop == BIDI_PROP_LRE ||
		    tmp_prop == BIDI_PROP_RLO ||
		    tmp_prop == BIDI_PROP_LRO ||
		    tmp_prop == BIDI_PROP_PDF ||
		    tmp_prop == BIDI_PROP_BN) {
			s.level = -1;
			state_serialize(&s, &(buf[bufoff]));
		}
	}

	/* X10 (W1-W7, N0-N2) */
	for (bufoff = 0; bufoff < buflen; bufoff++) {
		state_deserialize(buf[bufoff], &s);
		if (!s.visited && s.level != -1) {
			bufoff += process_isolating_run_sequence(buf, buflen, bufoff,
			                                         paragraph_level);
		}
	}

	/*
	 * I1-I2 (given our sequential approach to processing the
	 * isolating run sequences, we apply this rule separately)
	 */
	for (bufoff = 0; bufoff < buflen; bufoff++) {
		state_deserialize(buf[bufoff], &s);

		if (s.level % 2 == 0 ) {
			/* even level */
			if (s.prop == BIDI_PROP_R) {
				s.level += 1;
			} else if (s.prop == BIDI_PROP_AN ||
			           s.prop == BIDI_PROP_EN) {
				s.level += 2;
			}
		} else {
			/* odd level */
			if (s.prop == BIDI_PROP_L  ||
			    s.prop == BIDI_PROP_EN ||
			    s.prop == BIDI_PROP_AN) {
				s.level += 1;
			}
		}

		state_serialize(&s, &(buf[bufoff]));
	}

	/* L1 (rules 1-3) */
	runsince = SIZE_MAX;
	for (bufoff = 0; bufoff < buflen; bufoff++) {
		state_deserialize(buf[bufoff], &s);

		if (s.level == -1) {
			/* ignored character */
			continue;
		}

		if (s.rawprop == BIDI_PROP_WS  ||
		    s.rawprop == BIDI_PROP_FSI ||
		    s.rawprop == BIDI_PROP_LRI ||
		    s.rawprop == BIDI_PROP_RLI ||
		    s.rawprop == BIDI_PROP_PDI) {
			if (runsince == SIZE_MAX) {
				/* a new run has begun */
				runsince = bufoff;
			}
		} else if (s.rawprop == BIDI_PROP_S ||
		           s.rawprop == BIDI_PROP_B) {
			/* L1.4 -- ignored for now, < beachten! */
			for (i = runsince; i < bufoff; i++) {
				state_deserialize(buf[i], &t);
				if (t.level != -1) {
					t.level = (int_least8_t)paragraph_level;
					state_serialize(&t, &(buf[i]));
				}
			}
			runsince = SIZE_MAX;
		} else {
			/* sequence ended */
			runsince = SIZE_MAX;
		}

		if (s.rawprop == BIDI_PROP_S ||
		    s.rawprop == BIDI_PROP_B) {
			s.level = (int_least8_t)paragraph_level;
			state_serialize(&s, &(buf[bufoff]));
		}
		continue;
	}
	if (runsince != SIZE_MAX) {
		/*
		 * this is the end of the line and we
		 * are in a run
		 */
		for (i = runsince; i < buflen; i++) {
			state_deserialize(buf[i], &s);
			if (s.level != -1) {
				s.level = (int_least8_t)paragraph_level;
			}
			state_serialize(&s, &(buf[i]));
		}
		runsince = SIZE_MAX;
	}
}

static size_t
get_embedding_levels(HERODOTUS_READER *r, enum grapheme_bidirectional_override override,
                     int_least32_t *buf, size_t buflen)
{
	struct state s;
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
			s.paragraph_level = 0;
			s.level = 0;
			s.prop = get_bidi_property(cp);
			s.bracket_off = get_bidi_bracket_off(cp);
			s.visited = 0;
			s.rawprop = get_bidi_property(cp);
			state_serialize(&s, &(buf[bufoff]));
		}
	}
	bufsize = herodotus_reader_number_read(r);

	for (bufoff = 0, lastparoff = 0; bufoff < bufsize; bufoff++) {
		state_deserialize(buf[bufoff], &s);
		if (s.prop != BIDI_PROP_B && bufoff != bufsize - 1) {
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
		state_deserialize(buf[bufoff], &s);
		buf[bufoff] = s.level;
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
