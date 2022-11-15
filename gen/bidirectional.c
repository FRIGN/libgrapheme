/* See LICENSE file for copyright and license details. */
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

#define FILE_BIDI_BRACKETS  "data/BidiBrackets.txt"
#define FILE_BIDI_CLASS     "data/DerivedBidiClass.txt"
#define FILE_BIDI_MIRRORING "data/BidiMirroring.txt"

static const struct property_spec bidi_property[] = {
	{
		/* default */
		.enumname = "L",
		.file = FILE_BIDI_CLASS,
		.ucdname = "L",
	},
	{
		.enumname = "AL",
		.file = FILE_BIDI_CLASS,
		.ucdname = "AL",
	},
	{
		.enumname = "AN",
		.file = FILE_BIDI_CLASS,
		.ucdname = "AN",
	},
	{
		.enumname = "B",
		.file = FILE_BIDI_CLASS,
		.ucdname = "B",
	},
	{
		.enumname = "BN",
		.file = FILE_BIDI_CLASS,
		.ucdname = "BN",
	},
	{
		.enumname = "CS",
		.file = FILE_BIDI_CLASS,
		.ucdname = "CS",
	},
	{
		.enumname = "EN",
		.file = FILE_BIDI_CLASS,
		.ucdname = "EN",
	},
	{
		.enumname = "ES",
		.file = FILE_BIDI_CLASS,
		.ucdname = "ES",
	},
	{
		.enumname = "ET",
		.file = FILE_BIDI_CLASS,
		.ucdname = "ET",
	},
	{
		.enumname = "FSI",
		.file = FILE_BIDI_CLASS,
		.ucdname = "FSI",
	},
	{
		.enumname = "LRE",
		.file = FILE_BIDI_CLASS,
		.ucdname = "LRE",
	},
	{
		.enumname = "LRI",
		.file = FILE_BIDI_CLASS,
		.ucdname = "LRI",
	},
	{
		.enumname = "LRO",
		.file = FILE_BIDI_CLASS,
		.ucdname = "LRO",
	},
	{
		.enumname = "NSM",
		.file = FILE_BIDI_CLASS,
		.ucdname = "NSM",
	},
	{
		.enumname = "ON",
		.file = FILE_BIDI_CLASS,
		.ucdname = "ON",
	},
	{
		.enumname = "PDF",
		.file = FILE_BIDI_CLASS,
		.ucdname = "PDF",
	},
	{
		.enumname = "PDI",
		.file = FILE_BIDI_CLASS,
		.ucdname = "PDI",
	},
	{
		.enumname = "R",
		.file = FILE_BIDI_CLASS,
		.ucdname = "R",
	},
	{
		.enumname = "RLE",
		.file = FILE_BIDI_CLASS,
		.ucdname = "RLE",
	},
	{
		.enumname = "RLI",
		.file = FILE_BIDI_CLASS,
		.ucdname = "RLI",
	},
	{
		.enumname = "RLO",
		.file = FILE_BIDI_CLASS,
		.ucdname = "RLO",
	},
	{
		.enumname = "S",
		.file = FILE_BIDI_CLASS,
		.ucdname = "S",
	},
	{
		.enumname = "WS",
		.file = FILE_BIDI_CLASS,
		.ucdname = "WS",
	},
};

static struct {
	uint_least32_t cp_base;
	uint_least32_t cp_pair;
	char type;
} *b = NULL;

static size_t blen;

static int
bracket_callback(const char *file, char **field, size_t nfields, char *comment,
                 void *payload)
{
	(void)file;
	(void)comment;
	(void)payload;

	if (nfields < 3) {
		/* we have less than 3 fields, discard the line */
		return 0;
	}

	/* extend bracket pair array */
	if (!(b = realloc(b, (++blen) * sizeof(*b)))) {
		fprintf(stderr, "realloc: %s\n", strerror(errno));
		exit(1);
	}

	/* parse field data */
	hextocp(field[0], strlen(field[0]), &(b[blen - 1].cp_base));
	hextocp(field[1], strlen(field[1]), &(b[blen - 1].cp_pair));
	if (strlen(field[2]) != 1 ||
	    (field[2][0] != 'o' && field[2][0] != 'c')) {
		/* malformed line */
		return 1;
	} else {
		b[blen - 1].type = field[2][0];
	}

	return 0;
}

static void
post_process(struct properties *prop)
{
	size_t i;

	for (i = 0; i < blen; i++) {
		/*
		 * given the base property fits in 5 bits, we simply
		 * store the bracket-offset in the bits above that.
		 *
		 * All those properties that are not set here implicitly
		 * have offset 0, which we prepared to contain a stub
		 * for a character that is not a bracket.
		 */
		prop[b[i].cp_base].property |= (i << 5);
	}
}

static uint_least8_t
fill_missing(uint_least32_t cp)
{
	/* based on the @missing-properties in data/DerivedBidiClass.txt */
	if ((cp >= UINT32_C(0x0590) && cp <= UINT32_C(0x05FF)) ||
	    (cp >= UINT32_C(0x07C0) && cp <= UINT32_C(0x085F)) ||
	    (cp >= UINT32_C(0xFB1D) && cp <= UINT32_C(0xFB4F)) ||
	    (cp >= UINT32_C(0x10800) && cp <= UINT32_C(0x10CFF)) ||
	    (cp >= UINT32_C(0x10D40) && cp <= UINT32_C(0x10EBF)) ||
	    (cp >= UINT32_C(0x10F00) && cp <= UINT32_C(0x10F2F)) ||
	    (cp >= UINT32_C(0x10F70) && cp <= UINT32_C(0x10FFF)) ||
	    (cp >= UINT32_C(0x1E800) && cp <= UINT32_C(0x1EC6F)) ||
	    (cp >= UINT32_C(0x1ECC0) && cp <= UINT32_C(0x1ECFF)) ||
	    (cp >= UINT32_C(0x1ED50) && cp <= UINT32_C(0x1EDFF)) ||
	    (cp >= UINT32_C(0x1EF00) && cp <= UINT32_C(0x1EFFF))) {
		return 17; /* class R */
	} else if ((cp >= UINT32_C(0x0600) && cp <= UINT32_C(0x07BF)) ||
	           (cp >= UINT32_C(0x0860) && cp <= UINT32_C(0x08FF)) ||
	           (cp >= UINT32_C(0xFB50) && cp <= UINT32_C(0xFDCF)) ||
	           (cp >= UINT32_C(0xFDF0) && cp <= UINT32_C(0xFDFF)) ||
	           (cp >= UINT32_C(0xFE70) && cp <= UINT32_C(0xFEFF)) ||
	           (cp >= UINT32_C(0x10D00) && cp <= UINT32_C(0x10D3F)) ||
	           (cp >= UINT32_C(0x10EC0) && cp <= UINT32_C(0x10EFF)) ||
	           (cp >= UINT32_C(0x10F30) && cp <= UINT32_C(0x10F6F)) ||
	           (cp >= UINT32_C(0x1EC70) && cp <= UINT32_C(0x1ECBF)) ||
	           (cp >= UINT32_C(0x1ED00) && cp <= UINT32_C(0x1ED4F)) ||
	           (cp >= UINT32_C(0x1EE00) && cp <= UINT32_C(0x1EEFF))) {
		return 1; /* class AL */
	} else if (cp >= UINT32_C(0x20A0) && cp <= UINT32_C(0x20CF)) {
		return 8; /* class ET */
	} else {
		return 0; /* class L */
	}
}

int
main(int argc, char *argv[])
{
	size_t i;

	(void)argc;

	/*
	 * the first element in the bracket array is initialized to
	 * all-zeros, as we use the implicit 0-offset for all those
	 * codepoints that are not a bracket
	 */
	if (!(b = calloc(1, sizeof(*b)))) {
		fprintf(stderr, "calloc: %s\n", strerror(errno));
		exit(1);
	}
	parse_file_with_callback(FILE_BIDI_BRACKETS, bracket_callback, NULL);

	properties_generate_break_property(bidi_property, LEN(bidi_property),
	                                   fill_missing, NULL, post_process,
	                                   "bidi", argv[0]);

	printf("\nenum bracket_type {\n\tBIDI_BRACKET_NONE,\n\t"
	       "BIDI_BRACKET_OPEN,\n\tBIDI_BRACKET_CLOSE,\n};\n\n"
	       "struct bracket {\n\tenum bracket_type type;\n"
	       "\tuint_least32_t pair;\n};\n\n"
	       "static const struct bracket bidi_bracket[] = {\n");
	for (i = 0; i < blen; i++) {
		printf("\t{\n\t\t.type = %s,\n\t\t.pair = "
		       "UINT32_C(0x%06X),\n\t},\n",
		       (b[i].type == 'o') ? "BIDI_BRACKET_OPEN" :
		       (b[i].type == 'c') ? "BIDI_BRACKET_CLOSE" :
		                            "BIDI_BRACKET_NONE",
		       b[i].cp_pair);
	}
	printf("};\n");

	return 0;
}
