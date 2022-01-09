/* See LICENSE file for copyright and license details. */
#include <stdbool.h>
#include <stdint.h>

#include "../gen/character-test.h"
#include "util.h"

static bool
is_break(uint_least32_t cp0, uint_least32_t cp1, GRAPHEME_STATE *state)
{
	return grapheme_is_character_break(cp0, cp1, state);
}

int
main(int argc, char *argv[])
{
	(void)argc;

	return run_break_tests(is_break, character_test,
	                       LEN(character_test), argv[0]);
}
