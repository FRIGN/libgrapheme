/* See LICENSE file for copyright and license details. */
#include <stdbool.h>
#include <stdint.h>

#include "../gen/character-test.h"
#include "../grapheme.h"
#include "util.h"

int
main(int argc, char *argv[])
{
	(void)argc;

	return run_break_tests(grapheme_next_character_break,
	                       character_break_test,
	                       LEN(character_break_test), argv[0]);
}
