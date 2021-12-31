/* See LICENSE file for copyright and license details. */
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

#define FILE_EMOJI    "data/emoji-data.txt"
#define FILE_GRAPHEME "data/GraphemeBreakProperty.txt"

struct properties {
	uint_least8_t break_property;
};

struct property_spec {
	const char *enumname;
	const char *file;
	const char *ucdname;
};

static const struct property_spec break_property[] = {
	{
		.enumname = "OTHER",
		.file     = NULL,
		.ucdname  = NULL,
	},
	{
		.enumname = "CONTROL",
		.file     = FILE_GRAPHEME,
		.ucdname  = "Control",
	},
	{
		.enumname = "CR",
		.file     = FILE_GRAPHEME,
		.ucdname  = "CR",
	},
	{
		.enumname = "EXTEND",
		.file     = FILE_GRAPHEME,
		.ucdname  = "Extend",
	},
	{
		.enumname = "EXTENDED_PICTOGRAPHIC",
		.file     = FILE_EMOJI,
		.ucdname  = "Extended_Pictographic",
	},
	{
		.enumname = "HANGUL_L",
		.file     = FILE_GRAPHEME,
		.ucdname  = "L",
	},
	{
		.enumname = "HANGUL_V",
		.file     = FILE_GRAPHEME,
		.ucdname  = "V",
	},
	{
		.enumname = "HANGUL_T",
		.file     = FILE_GRAPHEME,
		.ucdname  = "T",
	},
	{
		.enumname = "HANGUL_LV",
		.file     = FILE_GRAPHEME,
		.ucdname  = "LV",
	},
	{
		.enumname = "HANGUL_LVT",
		.file     = FILE_GRAPHEME,
		.ucdname  = "LVT",
	},
	{
		.enumname = "LF",
		.file     = FILE_GRAPHEME,
		.ucdname  = "LF",
	},
	{
		.enumname = "PREPEND",
		.file     = FILE_GRAPHEME,
		.ucdname  = "Prepend",
	},
	{
		.enumname = "REGIONAL_INDICATOR",
		.file     = FILE_GRAPHEME,
		.ucdname  = "Regional_Indicator",
	},
	{
		.enumname = "SPACINGMARK",
		.file     = FILE_GRAPHEME,
		.ucdname  = "SpacingMark",
	},
	{
		.enumname = "ZWJ",
		.file     = FILE_GRAPHEME,
		.ucdname  = "ZWJ",
	},
};

static int
break_property_callback(char *file, char **field, size_t nfields,
                        char *comment, void *payload)
{
	/* prop always has the length 0x110000 */
	struct properties *prop = (struct properties *)payload;
	struct range r;
	size_t i;
	uint_least32_t cp;

	(void)comment;

	if (nfields < 2) {
		return 1;
	}

	for (i = 0; i < LEN(break_property); i++) {
		/* identify fitting file and identifier */
		if (break_property[i].file &&
		    !strcmp(break_property[i].file, file) &&
		    !strcmp(break_property[i].ucdname, field[1])) {
			/* parse range in first field */
			if (range_parse(field[0], &r)) {
				return 1;
			}

			/* apply to all codepoints in the range */
			for (cp = r.lower; cp <= r.upper; cp++) {
				if (prop[cp].break_property != 0) {
					fprintf(stderr, "break_property_callback: "
					        "property overlap\n");
					exit(1);
				}

				prop[cp].break_property = i;
			}

			break;
		}
	}

	return 0;
}

struct compressed_properties {
	size_t *offset;
	struct properties *data;
	size_t datalen;
};

void
compress_properties(const struct properties *prop,
                    struct compressed_properties *comp)
{
	uint_least32_t cp, i;

	/* initialization */
	if (!(comp->offset = malloc((size_t)0x110000 * sizeof(*(comp->offset))))) {
		fprintf(stderr, "malloc: %s\n", strerror(errno));
		exit(1);
	}
	comp->data = NULL;
	comp->datalen = 0;

	for (cp = 0; cp < 0x110000; cp++) {
		for (i = 0; i < comp->datalen; i++) {
			if (!memcmp(&(prop[cp]), &(comp->data[i]), sizeof(*prop))) {
				/* found a match! */
				comp->offset[cp] = i;
				break;
			}
		}
		if (i == comp->datalen) {
			/*
			 * found no matching properties-struct, so
			 * add current properties to data and add the
			 * offset in the offset-table
			 */
			if (!(comp->data = reallocarray(comp->data,
			                                ++(comp->datalen),
			                                sizeof(*(comp->data))))) {
				fprintf(stderr, "reallocarray: %s\n",
				        strerror(errno));
				exit(1);
			}
			memcpy(&(comp->data[comp->datalen - 1]), &(prop[cp]),
			       sizeof(*prop));
			comp->offset[cp] = comp->datalen - 1;
		}
	}
}

struct major_minor_properties {
	size_t *major;
	size_t *minor;
	size_t minorlen;
};

static double
get_major_minor_properties(const struct compressed_properties *comp,
                           struct major_minor_properties *mm)
{
	size_t i, j, compression_count = 0;

	/*
	 * we currently have an array comp->offset which maps the
	 * codepoints 0..0x110000 to offsets into comp->data.
	 * To improve cache-locality instead and allow a bit of
	 * compressing, instead of directly mapping a codepoint
	 * 0xAAAABB with comp->offset, we generate two arrays major
	 * and minor such that
	 *    comp->offset(0xAAAABB) == minor[major[0xAAAA] + 0xBB]
	 * This yields a major-array of length 2^16 and a minor array
	 * of variable length depending on how many common subsequences
	 * can be filtered out.
	 */

	/* initialize */
	if (!(mm->major = malloc((size_t)0x1100 * sizeof(*(mm->major))))) {
		fprintf(stderr, "malloc: %s\n", strerror(errno));
		exit(1);
	}
	mm->minor = NULL;
	mm->minorlen = 0;

	printf("#include <stdint.h>\n\n");

	for (i = 0; i < (size_t)0x1100; i++) {
		/*
		 * we now look at the cp-range (i << 8)..(i << 8 + 0xFF)
		 * and check if its corresponding offset-data already
		 * exists in minor (because then we just point there
		 * and need less storage)
		 */
		for (j = 0; j + 0xFF < mm->minorlen; j++) {
			if (!memcmp(&(comp->offset[i << 8]),
			            &(mm->minor[j]),
			            sizeof(*(comp->offset)) * 0x100)) {
				break;
			}
		}
		if (j + 0xFF < mm->minorlen) {
			/* found an index */
			compression_count++;
			mm->major[i] = j;
		} else {
			/*
			 * add "new" sequence to minor and point to it
			 * in major
			 */
			mm->minorlen += 0x100;
			if (!(mm->minor = reallocarray(mm->minor,
			                               mm->minorlen,
			                               sizeof(*(mm->minor))))) {
				fprintf(stderr, "reallocarray: %s\n",
				        strerror(errno));
				exit(1);
			}
			memcpy(&(mm->minor[mm->minorlen - 0x100]),
			       &(comp->offset[i << 8]),
			       sizeof(*(mm->minor)) * 0x100);
			mm->major[i] = mm->minorlen - 0x100;
		}
	}

	/* return compression ratio */
	return (double)compression_count / 0x1100 * 100;
}

static void
print_lookup_table(char *name, size_t *data, size_t datalen, size_t maxval)
{
	char *type;
	size_t i;

	type = (maxval <= (((1 << 16) - 1)) + 0xFF) ? "uint_least16_t" :
	                                          "uint_least32_t";

	printf("const %s %s[] = {\n\t", type, name);
	for (i = 0; i < datalen; i++) {
		printf("%zu", data[i]);
		if (i + 1 == datalen) {
			printf("\n");
		} else if ((i + 1) % 8 != 0) {
			printf(", ");
		} else {
			printf(",\n\t");
		}

	}
	printf("};\n");
}

int
main(int argc, char *argv[])
{
	struct compressed_properties comp;
	struct major_minor_properties mm;
	struct properties *prop;
	size_t i;

	(void)argc;

	/* allocate property buffer for all codepoints */
	if (!(prop = calloc(0x110000, sizeof(*prop)))) {
		fprintf(stderr, "calloc: %s\n", strerror(errno));
		exit(1);
	}

	/* extract properties */
	parse_file_with_callback(FILE_EMOJI, break_property_callback, prop);
	parse_file_with_callback(FILE_GRAPHEME, break_property_callback, prop);

	/*
	 * deduplicate by generating an array of offsets into prop where
	 * common data points at the same offset
	 */
	compress_properties(prop, &comp);

	/* generate major-minor-offset-tables */
	fprintf(stderr, "%s: compression-ratio: %.2f%%\n", argv[0],
	        get_major_minor_properties(&comp, &mm));

	/* print data */
	if (mm.minorlen < 0x100) {
		fprintf(stderr, "minor-array is too short.\n");
	}
	print_lookup_table("major", mm.major, 0x1100, mm.minorlen - 0x100);
	printf("\n");
	print_lookup_table("minor", mm.minor, mm.minorlen, comp.datalen);
	printf("\n");

	printf("enum break_property {\n");
	for (i = 0; i < LEN(break_property); i++) {
		printf("\tBREAK_PROP_%s,\n", break_property[i].enumname);
	}
	printf("\tNUM_BREAK_PROPS,\n};\n\n");
	printf("struct properties {\n\tenum break_property break_property;\n};\n\n");

	/* properties table */
	printf("const struct properties prop[] = {\n");
	for (i = 0; i < comp.datalen; i++) {
		printf("\t{ BREAK_PROP_%s },\n",
		       break_property[comp.data[i].break_property].enumname);
	}
	printf("};\n");

	/* free data */
	free(prop);
	free(comp.data);
	free(comp.offset);
	free(mm.major);
	free(mm.minor);

	return 0;
}
