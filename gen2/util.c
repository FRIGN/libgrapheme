/* See LICENSE file for copyright and license details. */
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

struct range {
	uint_least32_t lower;
	uint_least32_t upper;
};

static int
hextocp(const char *str, size_t len, uint_least32_t *cp)
{
	size_t i;
	int off;
	char relative;

	/* the maximum valid codepoint is 0x10FFFF */
	if (len > 6) {
		fprintf(stderr, "hextocp: '%.*s' is too long.\n", (int)len,
		        str);
		return 1;
	}

	for (i = 0, *cp = 0; i < len; i++) {
		if (str[i] >= '0' && str[i] <= '9') {
			relative = '0';
			off = 0;
		} else if (str[i] >= 'a' && str[i] <= 'f') {
			relative = 'a';
			off = 10;
		} else if (str[i] >= 'A' && str[i] <= 'F') {
			relative = 'A';
			off = 10;
		} else {
			fprintf(stderr, "hextocp: '%.*s' is not hexadecimal.\n",
			        (int)len, str);
			return 1;
		}

		*cp += ((uint_least32_t)1 << (4 * (len - i - 1))) *
		       (uint_least32_t)(str[i] - relative + off);
	}

	if (*cp > UINT32_C(0x10FFFF)) {
		fprintf(stderr, "hextocp: '%.*s' is too large.\n", (int)len,
		        str);
		return 1;
	}

	return 0;
}

int
parse_range(const char *str, struct range *range)
{
	char *p;

	if ((p = strstr(str, "..")) == NULL) {
		/* input has the form "XXXXXX" */
		if (hextocp(str, strlen(str), &range->lower)) {
			return 1;
		}
		range->upper = range->lower;
	} else {
		/* input has the form "XXXXXX..XXXXXX" */
		if (hextocp(str, (size_t)(p - str), &range->lower) ||
		    hextocp(p + 2, strlen(p + 2), &range->upper)) {
			return 1;
		}
	}

	return 0;
}

static bool
get_line(char **buf, size_t *bufsize, FILE *fp, size_t *len)
{
	int ret = EOF;

	for (*len = 0;; (*len)++) {
		if (*len > 0 && *buf != NULL && (*buf)[*len - 1] == '\n') {
			/*
			 * if the previously read character was a newline,
			 * we fake an end-of-file so we NUL-terminate and
			 * are done.
			 */
			ret = EOF;
		} else {
			ret = fgetc(fp);
		}

		if (*len >= *bufsize) {
			/* the buffer needs to be expanded */
			*bufsize += 512;
			if ((*buf = realloc(*buf, *bufsize)) == NULL) {
				fprintf(stderr, "get_line: Out of memory.\n");
				exit(1);
			}
		}

		if (ret != EOF) {
			(*buf)[*len] = (char)ret;
		} else {
			(*buf)[*len] = '\0';
			break;
		}
	}

	return *len == 0 && (feof(fp) || ferror(fp));
}

static char *
duplicate_string(const char *src)
{
	size_t len = strlen(src);
	char *dest;

	if (!(dest = malloc(len + 1))) {
		fprintf(stderr, "malloc: %s\n", strerror(errno));
		exit(1);
	}

	memcpy(dest, src, len);
	dest[len] = '\0';

	return dest;
}

void
duplicate_codepoint_property(const struct codepoint_property *src,
	struct codepoint_property *dest)
{
	size_t i;

	/* copy the field count */
	dest->field_count = src->field_count;

	/* allocate the field array */
	if (!(dest->fields = calloc(dest->field_count, sizeof(*(dest->fields))))) {
		fprintf(stderr, "malloc: %s\n", strerror(errno));
		exit(1);
	}

	for (i = 0; i < dest->field_count; i++) {
		dest->fields[i] = duplicate_string(src->fields[i]);
	}
}

static void
free_codepoint_property(struct codepoint_property *p)
{
	size_t i;

	for (i = 0; i < p->field_count; i++) {
		free(p->fields[i]);
	}
	free(p->fields);
}

static void
free_codepoint_property_set(struct codepoint_property_set *p)
{
	size_t i;

	for (i = 0; i < p->property_count; i++) {
		free_codepoint_property(&(p->properties[i]));
	}
	free(p->properties);
}

void
free_codepoint_property_set_array(struct codepoint_property_set *p)
{
	uint_least32_t cp;

	for (cp = 0; cp < UINT32_C(0x110000); cp++) {
		free_codepoint_property_set(&(p[cp]));
	}
	free(p);
}

const struct codepoint_property *
match_in_codepoint_property_set(const struct codepoint_property_set *p,
	const char *str, size_t field_offset)
{
	size_t i;

	for (i = 0; i < p->property_count; i++) {
		/* there must be enough fields to reach the offset */
		if (field_offset >= p->properties[i].field_count) {
			continue;
		}

		/* check for a match */
		if (strcmp(p->properties[i].fields[field_offset], str) == 0) {
			return &(p->properties[i]);
		}
	}
	
	return NULL;
}

struct codepoint_property_set *
parse_property_file(const char *fname)
{
	FILE *fp;
	struct codepoint_property p;
	struct codepoint_property_set *cpp, *missing;
	struct range range;
	char *line = NULL, **field = NULL, *comment;
	uint_least32_t cp;
	size_t linebufsize = 0, i, fieldbufsize = 0, j, nfields, len;
	bool is_missing;

	/*
	 * allocate cpp buffer of length 0x11000, initialised to zeros
	 * (NULL 'properties' pointer, zero property_count)
	 */
	if (!(cpp = calloc((size_t)UINT32_C(0x110000), sizeof(*cpp)))) {
		fprintf(stderr, "calloc: %s\n", strerror(errno));
		exit(1);
	}

	/*
	 * allocate same-sized array for 'missing' fields. The data files
	 * have this strange notion of defining some properties in bloody
	 * comments, and in a way that says 'yeah, if we did not define
	 * something for some, then replace it with this value'. However,
	 * it complicates things, as multiple, overlapping @missing
	 * can be defined in a single file. One can deduce that subsequent
	 * @missing just overwrite each other, but there's no way to
	 * track which properties have not been set without running
	 * through the file multiple times, which we want to avoid, so
	 * we accumulate the (self-overwriting) @missing set separately
	 * and fill it in at the end.
	 */
	if (!(missing = calloc((size_t)UINT32_C(0x110000), sizeof(*missing)))) {
		fprintf(stderr, "calloc: %s\n", strerror(errno));
		exit(1);
	}

	/* open the property file */
	if (!(fp = fopen(fname, "r"))) {
		fprintf(stderr, "parse_property_file: fopen '%s': %s.\n",
		        fname, strerror(errno));
		exit(1);
	}

	while (!get_line(&line, &linebufsize, fp, &len)) {
		/* remove trailing newline */
		if (len > 0 && line[len - 1] == '\n') {
			line[len - 1] = '\0';
			len--;
		}

		/* skip empty lines */
		if (len == 0) {
			continue;
		}

		/* comment, check if it's a @missing */
		is_missing = false;
		if (line[0] == '#') {
			if (strncmp(line + 1, " @missing: ", LEN(" @missing: ") - 1) == 0) {
				/*
				 * this comment specifies a 'missing' line.
				 * We shift it to the left and treat it
				 * like any other line, differentiating it
				 * with the 'is_missing' flag.
				 */
				size_t offset = 1 + (LEN(" @missing: ") - 1);

				memmove(line, line + offset, len - offset + 1);
				len -= offset;
				is_missing = true;
			} else {
				/* skip unrelated comment line */
				continue;
			}
		}

		/* tokenize line into fields */
		for (i = 0, nfields = 0, comment = NULL; i < (size_t)len; i++) {
			/* skip leading whitespace */
			while (line[i] == ' ') {
				i++;
			}

			/* check if we crashed into the comment */
			if (line[i] != '#') {
				/* extend field buffer, if necessary */
				if (++nfields > fieldbufsize) {
					if ((field = realloc(
						     field,
						     nfields *
							     sizeof(*field))) ==
					    NULL) {
						fprintf(stderr,
						        "parse_property_"
						        "file: realloc: "
						        "%s.\n",
						        strerror(errno));
						exit(1);
					}
					fieldbufsize = nfields;
				}

				/* set current position as field start */
				field[nfields - 1] = &line[i];

				/* continue until we reach ';' or '#' or end */
				while (line[i] != ';' && line[i] != '#' &&
				       line[i] != '\0') {
					i++;
				}
			}

			if (line[i] == '#') {
				/* set comment-variable for later */
				comment = &line[i + 1];
			}

			/* go back whitespace and terminate field there */
			if (i > 0) {
				for (j = i - 1; line[j] == ' '; j--) {
					;
				}
				line[j + 1] = '\0';
			} else {
				line[i] = '\0';
			}

			/* if comment is set, we are done */
			if (comment != NULL) {
				break;
			}
		}

		/* skip leading whitespace in comment */
		while (comment != NULL && comment[0] == ' ') {
			comment++;
		}

		/*
		 * We now have an array 'field' with 'nfields'. We can
		 * follow from the file format that, if nfields > 0,
		 * field[0] specifies a codepoint or range of
		 * codepoints, which we parse as the first step.
		 */
		if (nfields < 2) {
			/*
			 * either there is only a range or no range at
			 * all. Report this
			 */
			fprintf(stderr, "parse_property_file: malformed "
			        "line with either no range or no entry\n");
			exit(1);
		}

		if (parse_range(field[0], &range)) {
			fprintf(stderr, "parse_property_file: parse_range of "
			        "'%s' failed.\n", field[0]);
			exit(1);
		}

		/*
		 * fill a codepoint_property from the remaining
		 * fields (no allocations on a stack-allocated struct
		 */
		p.fields = field + 1;
		p.field_count = nfields - 1;

		/*
		 * add a duplicate of the filled codepoint_property
		 * to all codepoints in the range (i.e. the cpp or
		 * missing array, depending on is_missing)
		 */
		for (cp = range.lower; cp <= range.upper; cp++) {
			if (is_missing) {
				/*
				 * we overwrite the set at the codepoint,
				 * as the @missing properties overwrite
				 * each other (bad design)
				 */
				if (missing[cp].property_count == 0) {
					/*
					 * set is still empty, add space
					 * for one pointer to a
					 * codepoint_property
					 */
					if (!(missing[cp].properties =
					      malloc(sizeof(*(missing[cp].properties))))) {
						fprintf(stderr, "malloc: %s\n",
						        strerror(errno));
						exit(1);
					}
					missing[cp].property_count = 1;
				} else if (missing[cp].property_count == 1) {
					/* free the old property */
					free_codepoint_property(
						&(missing[cp].properties[0]));
				} else {
					/* this shouldn't happen */
					fprintf(stderr, "parse_property_file: "
						"malformed missing set\n");
					exit(1);
				}

				/* copy in the new one */
				duplicate_codepoint_property(
					&p, &(missing[cp].properties[0]));
			} else {
				/* expand the set by one element */
				if (!(cpp[cp].properties = realloc(
					cpp[cp].properties,
					sizeof(*(cpp[cp].properties)) *
					(++cpp[cp].property_count)))) {
					fprintf(stderr, "malloc: %s\n",
					        strerror(errno));
					exit(1);
				}

				duplicate_codepoint_property(&p,
					&(cpp[cp].properties[cpp[cp].property_count - 1]));
			}
		}
	}

	/*
	 * now we add the missing elements. We purposefully do not
	 * follow the interpretation in DerivedCoreProperties.txt for
	 * the @missing 'InCB; None'. Missing, for us, means that
	 * _no_ properties have been extracted and thus property_count
	 * is zero, not that some first field is not set. Absolute
	 * madness to even publish data like this...
	 */
	for (cp = 0; cp < UINT32_C(0x110000); cp++) {
		if (cpp[cp].property_count == 0) {
			/* swap the whole lot */
			cpp[cp].properties = missing[cp].properties;
			cpp[cp].property_count = missing[cp].property_count;
			missing[cp].properties = NULL;
			missing[cp].property_count = 0;
		}
	}

	/* close file */
	if (fclose(fp)) {
		fprintf(stderr, "parse_property_file: fclose '%s': %s.\n",
		        fname, strerror(errno));
		exit(1);
	}

	/* cleanup */
	free_codepoint_property_set_array(missing);
	free(line);
	free(field);

	/* return codepoint properties array */
	return cpp;
}

struct compression_output {
	uint_least64_t *major;
	uint_least64_t *minor;
	size_t block_size_shift;
	size_t block_size;
	size_t major_size;
	size_t minor_size;
	size_t major_entry_size;
	size_t minor_entry_size;
	size_t total_size;
};

static void
compress_array(const uint_least64_t *array, size_t block_size_shift,
               struct compression_output *co)
{
	size_t i, j, major_maximum, minor_maximum;

	/* set some preliminary data in the output struct */
	co->block_size_shift = block_size_shift;
	co->block_size = ((size_t)1) << block_size_shift;
	co->major_size = UINT32_C(0x110000) / co->block_size;

	/* allocate/initialise */
	if (!(co->major = malloc(co->major_size * sizeof(*(co->major))))) {
		fprintf(stderr, "malloc: %s\n", strerror(errno));
		exit(1);
	}
	co->minor = NULL;
	co->minor_size = 0;

	/* iterate over all blocks in the array */
	for (i = 0; i < co->major_size; i++) {
		size_t block_offset = i * co->block_size;

		/*
		 * iterate over all possible matching block starting
		 * positions in the minor array
		 */
		for (j = 0; j + co->block_size < co->minor_size; j++) {
			/*
			 * check if our current block matches the
			 * candidate block in the minor array
			 */
			if (memcmp(array + block_offset,
			           co->minor + j,
			           co->block_size * sizeof(*array)) == 0) {
				/*
				 * we have a match, so we don't have to
				 * store this block explicitly and just
				 * point to the offset in minor
				 */
				co->major[i] = j;
				break;
			}
		}
		if (j + co->block_size >= co->minor_size) {
			/*
			 * we found no matching subblock in minor. Add it
			 * to the minor array. The index to the first
			 * element we add now is exactly the size
			 * of the minor array.
			 */
			co->major[i] = co->minor_size;
			co->minor_size += co->block_size;
			if (!(co->minor = realloc(co->minor, co->minor_size *
			                          sizeof(*(co->minor))))) {
				fprintf(stderr, "malloc: %s\n",
				        strerror(errno));
				exit(1);
			}
			memcpy(co->minor + co->minor_size - co->block_size,
			       array + block_offset,
			       co->block_size * sizeof(*array));
		}
	}

	/* the compression is done. Now we derive some metadata */

	/* determine maxima */
	for (i = 0, major_maximum = 0; i < co->major_size; i++) {
		if (co->major[i] > major_maximum) {
			major_maximum = co->major[i];
		}
	}
	for (i = 0, minor_maximum = 0; i < co->minor_size; i++) {
		if (co->minor[i] > minor_maximum) {
			minor_maximum = co->minor[i];
		}
	}

	/* determine entry sizes */	
	if (major_maximum < UINT64_C(1) << 8) {
		co->major_entry_size = 8;
	} else if (major_maximum < UINT64_C(1) << 16) {
		co->major_entry_size = 16;
	} else if (major_maximum < UINT64_C(1) << 32) {
		co->major_entry_size = 32;
	} else {
		co->major_entry_size = 64;
	}

	if (minor_maximum < UINT64_C(1) << 4) {
		/* using 4 bit packed nibbles */
		co->minor_entry_size = 4;
	} else if (minor_maximum < UINT64_C(1) << 8) {
		co->minor_entry_size = 8;
	} else if (minor_maximum < UINT64_C(1) << 16) {
		co->minor_entry_size = 16;
	} else if (minor_maximum < UINT64_C(1) << 32) {
		co->minor_entry_size = 32;
	} else {
		co->minor_entry_size = 64;
	}

	/* total data size in bytes */
	co->total_size = ((co->major_size * co->major_entry_size) +
	                  (co->minor_size * co->minor_entry_size)) / 8;
}

void
free_compressed_data(struct compression_output *co)
{
	free(co->major);
	free(co->minor);
	memset(co, 0, sizeof(*co));
}

void
print_array(const uint_least64_t *array, size_t array_size,
            size_t array_entry_size, const char *prefix,
	    const char *name)
{
	size_t i;

	if (array_entry_size == 4) {
		/* we store two 4-bit nibbles in one byte */
		if (array_size % 2 != 0) {
			/* ensure array lenght is even */
			fprintf(stderr, "print_array: 4-bit array "
			        "is not implemented for odd length.");
			exit(1);
		}

		printf("static const uint_least8_t %s_%s[] = {",
		       prefix, name);
		for (i = 0; i < array_size; i += 2) {
			if ((i / 2) % 8 == 0) {
				printf("\n\t");
			} else {
				printf(" ");
			}

			/* write high and low nibbles */
			printf("%zu", (array[i] << 4) | array[i + 1]);

			if (i + 2 < array_size) {
				printf(",");
			}
		}
		printf("\n};\n");
	} else {
		printf("static const uint_least%zu_t %s_%s[] = {",
		       array_entry_size, prefix, name);
		for (i = 0; i < array_size; i++) {
			if (i % 8 == 0) {
				printf("\n\t");
			} else {
				printf(" ");
			}
			printf("%zu", array[i]);
			if (i + 1 < array_size) {
				printf(",");
			}
		}
		printf("\n};\n");
	}
}

void
compress_and_output(uint_least64_t *array, const char *prefix)
{
	struct compression_output co, co_best;
	size_t i, j, dictionary_size, dictionary_entry_size;
	uint_least64_t maximum = 0, *dictionary, *keys;

	/* initialise dictionary */
	dictionary = NULL;
	dictionary_size = 0;

	/* initialise keys */
	if (!(keys = calloc(UINT32_C(0x110000), sizeof(*keys)))) {
		fprintf(stderr, "calloc: %s\n", strerror(errno));
		exit(1);	
	}

	for (i = 0; i < UINT32_C(0x110000); i++) {
		/* maximum determination */
		if (array[i] > maximum) {
			maximum = array[i];
		}

		/* check if the array value is already in the dictionary */
		for (j = 0; j < dictionary_size; j++) {
			if (array[i] == dictionary[j]) {
				break;
			}
		}
		if (j == dictionary_size) {
			/* not in the dictionary, insert the array value */
			if (!(dictionary = realloc(dictionary, (++dictionary_size) *
				sizeof(*dictionary)))) {
				fprintf(stderr, "realloc: %s\n", strerror(errno));
				exit(1);	
			}
			dictionary[dictionary_size - 1] = array[i];
		}

		/* set the index (key) of the matched dictionary entry */
		keys[i] = j;
	}

	/* check if dictionary size is below a reasonable threshold */
	if (dictionary_size > 256) {
		fprintf(stderr, "compress_and_output: dictionary too large\n");
		exit(1);	
	}

	/*
	 * compress keys array with different block size shifts
	 * (block sizes from 16=1<<4 to 32768=1<<15)
	 */
	memset(&co_best, 0, sizeof(co_best));
	co_best.total_size = SIZE_MAX;
	for (i = 15; i >= 4; i--) {
		compress_array(keys, i, &co);

		fprintf(stderr, "compress_and_output: "
		        "block size %zu -> data size %.1f kB (%zu,%zu)\n",
		        ((size_t)1) << i, (double)co.total_size / 1000, co.major_entry_size, co.minor_entry_size);

		if (co.total_size < co_best.total_size) {
			/* we have a new best compression */
			free_compressed_data(&co_best);
			co_best = co;
		} else {
			/* not optimal, discard it */
			free_compressed_data(&co);
		}
	}
	fprintf(stderr, "compress_and_output: choosing optimal block size %zu (%zu,%zu)\n",
	        co_best.block_size, co_best.major_entry_size, co_best.minor_entry_size);

	/* output header */
	printf("/* Automatically generated by gen2/%s */\n"
	       "#include <stdint.h>\n\n"
	       "#include \"character.h\"\n\n", prefix);

	/* output dictionary */
	if (maximum < UINT64_C(1) << 8) {
		dictionary_entry_size = 8;
	} else if (maximum < UINT64_C(1) << 16) {
		dictionary_entry_size = 16;
	} else if (maximum < UINT64_C(1) << 32) {
		dictionary_entry_size = 32;
	} else {
		dictionary_entry_size = 64;
	}

	print_array(dictionary, dictionary_size, dictionary_entry_size,
	            prefix, "dictionary");
	printf("\n");

	/* output major array */
	print_array(co_best.major, co_best.major_size,
	            co_best.major_entry_size, prefix, "major");
	printf("\n");

	/* output minor array */
	print_array(co_best.minor, co_best.minor_size,
	            co_best.minor_entry_size, prefix, "minor");
	printf("\n");

	/* output accessor function (major is never 4-bits per entry) */
	printf("static inline uint_least%zu_t\n"
	       "get_%s_property(uint_least32_t cp)\n"
	       "{\n"
	       "\tuint_least%zu_t minor_offset = %s_major[cp >> %zu]\n"
	       "\t\t+ (cp & ((UINT32_C(1) << %zu) - 1));\n",
	       dictionary_entry_size, prefix, co_best.major_entry_size,
	       prefix, co_best.block_size_shift, co_best.block_size_shift);
	if (co_best.minor_entry_size == 4) {
		printf("\tuint_least8_t packed_value = %s_minor[minor_offset / 2];\n\n"
		       "\tif (minor_offset & UINT8_C(1) == 0) {\n"
		       "\t\t/* high nibble */\n"
		       "\t\treturn %s_dictionary[packed_value >> 4];\n"
		       "\t} else {\n"
		       "\t\t/* low nibble */\n"
		       "\t\treturn %s_dictionary[packed_value & UINT8_C(0x0f)];\n"
		       "\t}\n",
		       prefix, prefix, prefix);
	} else {
		printf("\n\treturn %s_dictionary[%s_minor[minor_offset]];\n",
		       prefix, prefix);
	}
	printf("}\n");

	/* cleanup */
	free(dictionary);
	free(keys);
}
