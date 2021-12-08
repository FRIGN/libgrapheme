/* See LICENSE file for copyright and license details. */
#include <stddef.h>

#include "util.h"

#define FILE_EMOJI    "data/emoji-data.txt"
#define FILE_GRAPHEME "data/GraphemeBreakProperty.txt"

static struct property segment_property[] = {
	{
		.enumname   = "GRAPHEME_PROP_CONTROL",
		.identifier = "Control",
		.fname      = FILE_GRAPHEME,
	},
	{
		.enumname   = "GRAPHEME_PROP_CR",
		.identifier = "CR",
		.fname      = FILE_GRAPHEME,
	},
	{
		.enumname   = "GRAPHEME_PROP_EXTEND",
		.identifier = "Extend",
		.fname      = FILE_GRAPHEME,
	},
	{
		.enumname   = "GRAPHEME_PROP_EXTENDED_PICTOGRAPHIC",
		.identifier = "Extended_Pictographic",
		.fname      = FILE_EMOJI,
	},
	{
		.enumname   = "GRAPHEME_PROP_HANGUL_L",
		.identifier = "L",
		.fname      = FILE_GRAPHEME,
	},
	{
		.enumname   = "GRAPHEME_PROP_HANGUL_V",
		.identifier = "V",
		.fname      = FILE_GRAPHEME,
	},
	{
		.enumname   = "GRAPHEME_PROP_HANGUL_T",
		.identifier = "T",
		.fname      = FILE_GRAPHEME,
	},
	{
		.enumname   = "GRAPHEME_PROP_HANGUL_LV",
		.identifier = "LV",
		.fname      = FILE_GRAPHEME,
	},
	{
		.enumname   = "GRAPHEME_PROP_HANGUL_LVT",
		.identifier = "LVT",
		.fname      = FILE_GRAPHEME,
	},
	{
		.enumname   = "GRAPHEME_PROP_LF",
		.identifier = "LF",
		.fname      = FILE_GRAPHEME,
	},
	{
		.enumname   = "GRAPHEME_PROP_PREPEND",
		.identifier = "Prepend",
		.fname      = FILE_GRAPHEME,
	},
	{
		.enumname   = "GRAPHEME_PROP_REGIONAL_INDICATOR",
		.identifier = "Regional_Indicator",
		.fname      = FILE_GRAPHEME,
	},
	{
		.enumname   = "GRAPHEME_PROP_SPACINGMARK",
		.identifier = "SpacingMark",
		.fname      = FILE_GRAPHEME,
	},
	{
		.enumname   = "GRAPHEME_PROP_ZWJ",
		.identifier = "ZWJ",
		.fname      = FILE_GRAPHEME,
	},
};

int
main(int argc, char *argv[])
{
	(void)argc;

	property_list_parse(segment_property, LEN(segment_property));
	property_list_print(segment_property, LEN(segment_property),
	                    "grapheme_prop", argv[0]);

	return 0;
}
