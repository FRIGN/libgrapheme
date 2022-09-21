/* See LICENSE file for copyright and license details. */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../grapheme.h"
#include "util.h"

struct unit_test_to_case_utf8 {
	const char *description;
	struct {
		const char *src;
		size_t srclen;
		size_t destlen;
	} input;
	struct {
		const char *dest;
		size_t ret;
	} output;
};

struct unit_test_to_case_utf8 lowercase_utf8[] = {
	{
		.description = "empty input",
		.input =  { "", 0, 10 },
		.output = { "", 0 },
	},
	{
		.description = "empty output",
		.input =  { "hello", 5, 0 },
		.output = { "", 5 },
	},
	{
		.description = "one character, conversion",
		.input =  { "A", 1, 10 },
		.output = { "a", 1 },
	},
	{
		.description = "one character, no conversion",
		.input =  { "a", 1, 10 },
		.output = { "a", 1 },
	},
	{
		.description = "one character, conversion, truncation",
		.input =  { "A", 1, 0 },
		.output = { "", 1 },
	},
	{
		.description = "one character, conversion, NUL-terminated",
		.input =  { "A", SIZE_MAX, 10 },
		.output = { "a", 1 },
	},
	{
		.description = "one character, no conversion, NUL-terminated",
		.input =  { "a", SIZE_MAX, 10 },
		.output = { "a", 1 },
	},
	{
		.description = "one character, conversion, NUL-terminated, truncation",
		.input =  { "A", SIZE_MAX, 0 },
		.output = { "", 1 },
	},
	{
		.description = "one word, conversion",
		.input =  { "wOrD", 4, 10 },
		.output = { "word", 4 },
	},
	{
		.description = "one word, no conversion",
		.input =  { "word", 4, 10 },
		.output = { "word", 4 },
	},
	{
		.description = "one word, conversion, truncation",
		.input =  { "wOrD", 4, 3 },
		.output = { "wo", 4 },
	},
	{
		.description = "one word, conversion, NUL-terminated",
		.input =  { "wOrD", SIZE_MAX, 10 },
		.output = { "word", 4 },
	},
	{
		.description = "one word, no conversion, NUL-terminated",
		.input =  { "word", SIZE_MAX, 10 },
		.output = { "word", 4 },
	},
	{
		.description = "one word, conversion, NUL-terminated, truncation",
		.input =  { "wOrD", SIZE_MAX, 3 },
		.output = { "wo", 4 },
	},
};

struct unit_test_to_case_utf8 uppercase_utf8[] = {
	{
		.description = "empty input",
		.input =  { "", 0, 10 },
		.output = { "", 0 },
	},
	{
		.description = "empty output",
		.input =  { "hello", 5, 0 },
		.output = { "", 5 },
	},
	{
		.description = "one character, conversion",
		.input =  { "a", 1, 10 },
		.output = { "A", 1 },
	},
	{
		.description = "one character, no conversion",
		.input =  { "A", 1, 10 },
		.output = { "A", 1 },
	},
	{
		.description = "one character, conversion, truncation",
		.input =  { "a", 1, 0 },
		.output = { "", 1 },
	},
	{
		.description = "one character, conversion, NUL-terminated",
		.input =  { "a", SIZE_MAX, 10 },
		.output = { "A", 1 },
	},
	{
		.description = "one character, no conversion, NUL-terminated",
		.input =  { "A", SIZE_MAX, 10 },
		.output = { "A", 1 },
	},
	{
		.description = "one character, conversion, NUL-terminated, truncation",
		.input =  { "a", SIZE_MAX, 0 },
		.output = { "", 1 },
	},
	{
		.description = "one word, conversion",
		.input =  { "wOrD", 4, 10 },
		.output = { "WORD", 4 },
	},
	{
		.description = "one word, no conversion",
		.input =  { "WORD", 4, 10 },
		.output = { "WORD", 4 },
	},
	{
		.description = "one word, conversion, truncation",
		.input =  { "wOrD", 4, 3 },
		.output = { "WO", 4 },
	},
	{
		.description = "one word, conversion, NUL-terminated",
		.input =  { "wOrD", SIZE_MAX, 10 },
		.output = { "WORD", 4 },
	},
	{
		.description = "one word, no conversion, NUL-terminated",
		.input =  { "WORD", SIZE_MAX, 10 },
		.output = { "WORD", 4 },
	},
	{
		.description = "one word, conversion, NUL-terminated, truncation",
		.input =  { "wOrD", SIZE_MAX, 3 },
		.output = { "WO", 4 },
	},
};

struct unit_test_to_case_utf8 titlecase_utf8[] = {
	{
		.description = "empty input",
		.input =  { "", 0, 10 },
		.output = { "", 0 },
	},
	{
		.description = "empty output",
		.input =  { "hello", 5, 0 },
		.output = { "", 5 },
	},
	{
		.description = "one character, conversion",
		.input =  { "a", 1, 10 },
		.output = { "A", 1 },
	},
	{
		.description = "one character, no conversion",
		.input =  { "A", 1, 10 },
		.output = { "A", 1 },
	},
	{
		.description = "one character, conversion, truncation",
		.input =  { "a", 1, 0 },
		.output = { "", 1 },
	},
	{
		.description = "one character, conversion, NUL-terminated",
		.input =  { "a", SIZE_MAX, 10 },
		.output = { "A", 1 },
	},
	{
		.description = "one character, no conversion, NUL-terminated",
		.input =  { "A", SIZE_MAX, 10 },
		.output = { "A", 1 },
	},
	{
		.description = "one character, conversion, NUL-terminated, truncation",
		.input =  { "a", SIZE_MAX, 0 },
		.output = { "", 1 },
	},
	{
		.description = "one word, conversion",
		.input =  { "heLlo", 5, 10 },
		.output = { "Hello", 5 },
	},
	{
		.description = "one word, no conversion",
		.input =  { "Hello", 5, 10 },
		.output = { "Hello", 5 },
	},
	{
		.description = "one word, conversion, truncation",
		.input =  { "heLlo", 5, 2 },
		.output = { "H", 5 },
	},
	{
		.description = "one word, conversion, NUL-terminated",
		.input =  { "heLlo", SIZE_MAX, 10 },
		.output = { "Hello", 5 },
	},
	{
		.description = "one word, no conversion, NUL-terminated",
		.input =  { "Hello", SIZE_MAX, 10 },
		.output = { "Hello", 5 },
	},
	{
		.description = "one word, conversion, NUL-terminated, truncation",
		.input =  { "heLlo", SIZE_MAX, 3 },
		.output = { "He", 5 },
	},
	{
		.description = "two words, conversion",
		.input =  { "heLlo wORLd!", 12, 20 },
		.output = { "Hello World!", 12 },
	},
	{
		.description = "two words, no conversion",
		.input =  { "Hello World!", 12, 20 },
		.output = { "Hello World!", 12 },
	},
	{
		.description = "two words, conversion, truncation",
		.input =  { "heLlo wORLd!", 12, 8 },
		.output = { "Hello W", 12 },
	},
	{
		.description = "two words, conversion, NUL-terminated",
		.input =  { "heLlo wORLd!", SIZE_MAX, 20 },
		.output = { "Hello World!", 12 },
	},
	{
		.description = "two words, no conversion, NUL-terminated",
		.input =  { "Hello World!", SIZE_MAX, 20 },
		.output = { "Hello World!", 12 },
	},
	{
		.description = "two words, conversion, NUL-terminated, truncation",
		.input =  { "heLlo wORLd!", SIZE_MAX, 4 },
		.output = { "Hel", 12 },
	},
};

static int
unit_test_callback_to_case_utf8(void *t, size_t off, const char *name, const char *argv0)
{
	struct unit_test_to_case_utf8 *test = (struct unit_test_to_case_utf8 *)t + off;
	size_t ret = 0, i;
	char buf[512];

	/* fill the array with canary values */
	memset(buf, 0x7f, LEN(buf));

	if (t == lowercase_utf8) {
		ret = grapheme_to_lowercase_utf8(test->input.src, test->input.srclen,
		                                 buf, test->input.destlen);
	} else if (t == uppercase_utf8) {
		ret = grapheme_to_uppercase_utf8(test->input.src, test->input.srclen,
		                                 buf, test->input.destlen);
	} else if (t == titlecase_utf8) {
		ret = grapheme_to_titlecase_utf8(test->input.src, test->input.srclen,
		                                 buf, test->input.destlen);
	} else {
		goto err;
	}

	/* check results */
	if (ret != test->output.ret ||
	    memcmp(buf, test->output.dest, MIN(test->input.destlen, test->output.ret))) {
		goto err;
	}

	/* check that none of the canary values have been overwritten */
	for (i = test->input.destlen; i < LEN(buf); i++) {
		if (buf[i] != 0x7f) {
fprintf(stderr, "REEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE\n");
			goto err;
		}
	}

	return 0;
err:
	fprintf(stderr, "%s: %s: Failed unit test %zu \"%s\" "
	        "(returned (\"%.*s\", %zu) instead of (\"%.*s\", %zu)).\n", argv0,
	        name, off, test->description, (int)ret, buf, ret,
	        (int)test->output.ret, test->output.dest, test->output.ret);
	return 1;
}

int
main(int argc, char *argv[])
{
	(void)argc;

	return run_unit_tests(unit_test_callback_to_case_utf8, lowercase_utf8,
	                      LEN(lowercase_utf8), "grapheme_to_lowercase_utf8", argv[0]) +
	       run_unit_tests(unit_test_callback_to_case_utf8, uppercase_utf8,
	                      LEN(uppercase_utf8), "grapheme_to_uppercase_utf8", argv[0]) +
	       run_unit_tests(unit_test_callback_to_case_utf8, titlecase_utf8,
	                      LEN(titlecase_utf8), "grapheme_to_titlecase_utf8", argv[0]);
}
