/* See LICENSE file for copyright and license details. */
#include <stddef.h>

#include "util.h"

int
main(int argc, char *argv[])
{
	struct segment_test *st = NULL;
	size_t numsegtests = 0;

	(void)argc;

	segment_test_list_parse("data/GraphemeBreakTest.txt", &st, &numsegtests);
	segment_test_list_print(st, numsegtests, "character_test", argv[0]);
	segment_test_list_free(st, numsegtests);

	return 0;
}
