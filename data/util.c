/* See LICENSE file for copyright and license details. */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "util.h"

void
parse_input(int (*process_line)(char **, size_t, char *))
{
	char *line = NULL, **field = NULL, *comment;
	size_t linebufsize = 0, i, fieldbufsize = 0, j, nfields;
	ssize_t len;

	while ((len = getline(&line, &linebufsize, stdin)) >= 0) {
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
					fprintf(stderr, "realloc: %s\n", strerror(errno));
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
			if (line [i] == '#') {
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

		/* call line processing function */
		if (process_line(field, nfields, comment)) {
			exit(1);
		}
	}

	free(line);
	free(field);
}

static int
valid_hexstring(const char *str)
{
	const char *p = str;

	while ((*p >= '0' && *p <= '9') ||
	       (*p >= 'a' && *p <= 'f') ||
	       (*p >= 'A' && *p <= 'F')) {
		p++;
	}

	if (*p != '\0') {
		fprintf(stderr, "invalid code point range '%s'\n", str);
		return 0;
	}

	return 1;
}

int
cp_parse(const char *str, uint32_t *cp)
{
	if (!valid_hexstring(str)) {
		return 1;
	}
	*cp = strtol(str, NULL, 16);

	return 0;
}

int
range_parse(const char *str, struct range *range)
{
	char *p;

	if ((p = strstr(str, "..")) == NULL) {
		/* input has the form "XXXXXX" */
		if (!valid_hexstring(str)) {
			return 1;
		}
		range->lower = range->upper = strtol(str, NULL, 16);
	} else {
		/* input has the form "XXXXXX..XXXXXX" */
		*p = '\0';
		p += 2;
		if (!valid_hexstring(str) || !valid_hexstring(p)) {
			return 1;
		}
		range->lower = strtol(str, NULL, 16);
		range->upper = strtol(p, NULL, 16);
	}

	return 0;
}

void
range_list_append(struct range **range, size_t *nranges, const struct range *new)
{
	if (*nranges > 0 && (*range)[*nranges - 1].upper == new->lower) {
		/* we can merge with previous entry */
		(*range)[*nranges - 1].upper = new->upper;
	} else {
		/* need to append new entry */
		if ((*range = realloc(*range, (++(*nranges)) * sizeof(**range))) == NULL) {
			fprintf(stderr, "realloc: %s\n", strerror(errno));
			exit(1);
		}
		(*range)[*nranges - 1].lower = new->lower;
		(*range)[*nranges - 1].upper = new->upper;
	}
}
