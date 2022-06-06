/* See LICENSE file for copyright and license details. */
#include <stdbool.h>
#include <stdint.h>

#include "../gen/word-test.h"
#include "../grapheme.h"
#include "util.h"

int
main(int argc, char *argv[])
{
	(void)argc;

	return run_break_tests(grapheme_next_word_break, word_break_test,
	                       LEN(word_break_test), argv[0]);
}
