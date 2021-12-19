/* See LICENSE file for copyright and license details. */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "util.h"

struct property_list_payload
{
	struct property *prop;
	size_t numprops;
};

struct segment_test_payload
{
	struct segment_test **st;
	size_t *numsegtests;
};

static int
hextocp(const char *str, size_t len, uint_least32_t *cp)
{
	size_t i;
	int off;
	char relative;

	/* the maximum valid codepoint is 0x10FFFF */
	if (len > 6) {
		fprintf(stderr, "hextocp: '%.*s' is too long.\n",
		        (int)len, str);
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

	if (*cp > 0x10ffff) {
		fprintf(stderr, "hextocp: '%.*s' is too large.\n",
		        (int)len, str);
		return 1;
	}

	return 0;
}

static int
range_parse(const char *str, struct range *range)
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

static void
range_list_append(struct range **range, size_t *nranges, const struct range *new)
{
	if (*nranges > 0 && (*range)[*nranges - 1].upper == new->lower) {
		/* we can merge with previous entry */
		(*range)[*nranges - 1].upper = new->upper;
	} else {
		/* need to append new entry */
		if ((*range = realloc(*range, (++(*nranges)) *
		                      sizeof(**range))) == NULL) {
			fprintf(stderr, "range_list_append: realloc: %s.\n",
			        strerror(errno));
			exit(1);
		}
		(*range)[*nranges - 1].lower = new->lower;
		(*range)[*nranges - 1].upper = new->upper;
	}
}

static void
parse_file_with_callback(char *fname, int (*callback)(char *, char **,
                         size_t, char *, void *), void *payload)
{
	FILE *fp;
	char *line = NULL, **field = NULL, *comment;
	size_t linebufsize = 0, i, fieldbufsize = 0, j, nfields;
	ssize_t len;

	/* open file */
	if (!(fp = fopen(fname, "r"))) {
		fprintf(stderr, "parse_file_with_callback: fopen '%s': %s.\n",
		        fname, strerror(errno));
		exit(1);
	}

	while ((len = getline(&line, &linebufsize, fp)) >= 0) {
		/* remove trailing newline */
		if (len > 0 && line[len - 1] == '\n') {
			line[len - 1] = '\0';
			len--;
		}

		/* skip empty lines and comment lines */
		if (len == 0 || line[0] == '#') {
			continue;
		}

		/* tokenize line into fields */
		for (i = 0, nfields = 0, comment = NULL; i < (size_t)len; i++) {
			/* extend field buffer, if necessary */
			if (++nfields > fieldbufsize) {
				if ((field = realloc(field, nfields *
                                     sizeof(*field))) == NULL) {
					fprintf(stderr, "parse_file_with_"
					        "callback: realloc: %s.\n",
					        strerror(errno));
					exit(1);
				}
				fieldbufsize = nfields;
			}

			/* skip leading whitespace */
			while (line[i] == ' ') {
				i++;
			}

			/* set current position as field start */
			field[nfields - 1] = &line[i];

			/* continue until we reach ';' or '#' or end */
			while (line[i] != ';' && line[i] != '#' &&
			       line[i] != '\0') {
				i++;
			}
			if (line[i] == '#') {
				/* set comment-variable for later */
				comment = &line[i + 1];
			}

			/* go back whitespace and terminate field there */
			if (i > 0) {
				for (j = i - 1; line[j] == ' '; j--)
					;
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

		/* call callback function */
		if (callback(fname, field, nfields, comment, payload)) {
			fprintf(stderr, "parse_file_with_callback: "
			        "Malformed input.\n");
			exit(1);
		}
	}

	free(line);
	free(field);
}

static int
property_list_callback(char *fname, char **field, size_t nfields,
                       char *comment, void *payload)
{
	struct property *prop = ((struct property_list_payload *)payload)->prop;
	struct range r;
	size_t i, numprops = ((struct property_list_payload *)payload)->numprops;

	(void)comment;

	if (nfields < 2) {
		return 1;
	}

	for (i = 0; i < numprops; i++) {
		if (!strcmp(field[1], prop[i].identifier) &&
		    !strcmp(fname, prop[i].fname)) {
			if (range_parse(field[0], &r)) {
				return 1;
			}
			range_list_append(&(prop[i].table),
			                  &(prop[i].tablelen), &r);
			break;
		}
	}

	return 0;
}

void
property_list_parse(struct property *prop, size_t numprops)
{
	struct property_list_payload pl = {
		.prop = prop,
		.numprops = numprops
	};
	size_t i;

	/* make sure to parse each file only once */
	for (i = 0; i < numprops; i++) {
		if (prop[i].tablelen > 0) {
			/* property's file was already parsed */
			continue;
		}

		parse_file_with_callback(prop[i].fname,
		                         property_list_callback, &pl);
	}
}

void
property_list_print(const struct property *prop, size_t numprops,
                    const char *identifier, const char *progname)
{
	size_t i, j;

	printf("/* Automatically generated by %s */\n"
	       "#include <stdint.h>\n\n#include \"../src/util.h\"\n\n",
	       progname);

	/* print enum */
	printf("enum %s {\n", identifier);
	for (i = 0; i < numprops; i++) {
		printf("\t%s,\n", prop[i].enumname);
	}
	printf("};\n\n");

	/* print table */
	printf("static const struct range_list %s[] = {\n", identifier);
	for (i = 0; i < numprops; i++) {
		printf("\t[%s] = {\n\t\t.data = (struct range[]){\n",
		       prop[i].enumname);
		for (j = 0; j < prop[i].tablelen; j++) {
			printf("\t\t\t{ UINT32_C(0x%06X), UINT32_C(0x%06X) },\n",
			       prop[i].table[j].lower,
			       prop[i].table[j].upper);
		}
		printf("\t\t},\n\t\t.len = %zu,\n\t},\n", prop[i].tablelen);
	}
	printf("};\n");
}

void
property_list_free(struct property *prop, size_t numprops)
{
	size_t i;

	for (i = 0; i < numprops; i++) {
		free(prop[i].table);
		prop[i].table = NULL;
		prop[i].tablelen = 0;
	}
}

static int
segment_test_callback(char *fname, char **field, size_t nfields,
                      char *comment, void *payload)
{
	struct segment_test *t,
		**test = ((struct segment_test_payload *)payload)->st;
	size_t i, *ntests = ((struct segment_test_payload *)payload)->numsegtests;
	char *token;

	(void)fname;

	if (nfields < 1) {
		return 1;
	}

	/* append new testcase and initialize with zeroes */
	if ((*test = realloc(*test, ++(*ntests) * sizeof(**test))) == NULL) {
		fprintf(stderr, "segment_test_callback: realloc: %s.\n",
		        strerror(errno));
		return 1;
	}
	t = &(*test)[*ntests - 1];
	memset(t, 0, sizeof(*t));

	/* parse testcase "<÷|×> <cp> <÷|×> ... <cp> <÷|×>" */
	for (token = strtok(field[0], " "), i = 0; token != NULL; i++,
	     token = strtok(NULL, " ")) {
		if (i % 2 == 0) {
			/* delimiter */
			if (!strncmp(token, "\xC3\xB7", 2)) { /* UTF-8 */
				/*
				 * '÷' indicates a breakpoint,
				 * the current length is done; allocate
				 * a new length field and set it to 0
				 */
				if ((t->len = realloc(t->len,
				     ++t->lenlen * sizeof(*t->len))) == NULL) {
					fprintf(stderr, "segment_test_"
					        "callback: realloc: %s.\n",
					        strerror(errno));
					return 1;
				}
				t->len[t->lenlen - 1] = 0;
			} else if (!strncmp(token, "\xC3\x97", 2)) { /* UTF-8 */
				/*
				 * '×' indicates a non-breakpoint, do nothing
				 */
			} else {
				fprintf(stderr, "segment_test_callback: "
				        "Malformed delimiter '%s'.\n", token);
				return 1;
			}
		} else {
			/* add codepoint to cp-array */
			if ((t->cp = realloc(t->cp, ++t->cplen *
			                     sizeof(*t->cp))) == NULL) {
				fprintf(stderr, "segment_test_callback: "
				        "realloc: %s.\n", strerror(errno));
				return 1;
			}
			if (hextocp(token, strlen(token), &t->cp[t->cplen - 1])) {
				return 1;
			}
			if (t->lenlen > 0) {
				t->len[t->lenlen - 1]++;
			}
		}
	}
	if (t->len[t->lenlen - 1] == 0) {
		/* we allocated one more length than we needed */
		t->lenlen--;
	}

	/* store comment */
	if (((*test)[*ntests - 1].descr = strdup(comment)) == NULL) {
		fprintf(stderr, "segment_test_callback: strdup: %s.\n",
		        strerror(errno));
		return 1;
	}

	return 0;
}

void
segment_test_list_parse(char *fname, struct segment_test **st,
                        size_t *numsegtests)
{
	struct segment_test_payload pl = {
		.st = st,
		.numsegtests = numsegtests
	};
	*st = NULL;
	*numsegtests = 0;

	parse_file_with_callback(fname, segment_test_callback, &pl);
}

void
segment_test_list_print(const struct segment_test *st, size_t numsegtests,
                        const char *identifier, const char *progname)
{
	size_t i, j;

	printf("/* Automatically generated by %s */\n"
	       "#include <stdint.h>\n#include <stddef.h>\n\n", progname);

	printf("static const struct {\n\tuint_least32_t *cp;\n"
	       "\tsize_t cplen;\n\tsize_t *len;\n\tsize_t lenlen;\n"
	       "\tchar *descr;\n} %s[] = {\n", identifier);
	for (i = 0; i < numsegtests; i++) {
		printf("\t{\n");

		printf("\t\t.cp     = (uint_least32_t[]){");
		for (j = 0; j < st[i].cplen; j++) {
			printf(" UINT32_C(0x%06X)", st[i].cp[j]);
			if (j + 1 < st[i].cplen) {
				putchar(',');
			}
		}
		printf(" },\n");
		printf("\t\t.cplen  = %zu,\n", st[i].cplen);

		printf("\t\t.len    = (size_t[]){");
		for (j = 0; j < st[i].lenlen; j++) {
			printf(" %zu", st[i].len[j]);
			if (j + 1 < st[i].lenlen) {
				putchar(',');
			}
		}
		printf(" },\n");
		printf("\t\t.lenlen = %zu,\n", st[i].lenlen);

		printf("\t\t.descr  = \"%s\",\n", st[i].descr);

		printf("\t},\n");
	}
	printf("};\n");
}

void
segment_test_list_free(struct segment_test *st, size_t numsegtests)
{
	(void)numsegtests;

	free(st);
}
