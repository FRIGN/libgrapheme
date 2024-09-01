/* See LICENSE file for copyright and license details. */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

#define FILE_DCP      "data/DerivedCoreProperties.txt"
#define FILE_EMOJI    "data/emoji-data.txt"
#define FILE_GRAPHEME "data/GraphemeBreakProperty.txt"

static const struct property_spec char_break_property[] = {
	{
		.enumname = "OTHER",
		.file = NULL,
		.ucdname = NULL,
	},
	{
		.enumname = "BOTH_EXTEND_ICB_EXTEND",
		.file = NULL,
		.ucdname = NULL,
	},
	{
		.enumname = "BOTH_EXTEND_ICB_LINKER",
		.file = NULL,
		.ucdname = NULL,
	},
	{
		.enumname = "BOTH_ZWJ_ICB_EXTEND",
		.file = NULL,
		.ucdname = NULL,
	},
	{
		.enumname = "CONTROL",
		.file = FILE_GRAPHEME,
		.ucdname = "Control",
	},
	{
		.enumname = "CR",
		.file = FILE_GRAPHEME,
		.ucdname = "CR",
	},
	{
		.enumname = "EXTEND",
		.file = FILE_GRAPHEME,
		.ucdname = "Extend",
	},
	{
		.enumname = "EXTENDED_PICTOGRAPHIC",
		.file = FILE_EMOJI,
		.ucdname = "Extended_Pictographic",
	},
	{
		.enumname = "HANGUL_L",
		.file = FILE_GRAPHEME,
		.ucdname = "L",
	},
	{
		.enumname = "HANGUL_V",
		.file = FILE_GRAPHEME,
		.ucdname = "V",
	},
	{
		.enumname = "HANGUL_T",
		.file = FILE_GRAPHEME,
		.ucdname = "T",
	},
	{
		.enumname = "HANGUL_LV",
		.file = FILE_GRAPHEME,
		.ucdname = "LV",
	},
	{
		.enumname = "HANGUL_LVT",
		.file = FILE_GRAPHEME,
		.ucdname = "LVT",
	},
	{
		.enumname = "ICB_CONSONANT",
		.file = FILE_DCP,
		.ucdname = "InCB",
		.ucdsubname = "Consonant",
	},
	{
		.enumname = "ICB_EXTEND",
		.file = FILE_DCP,
		.ucdname = "InCB",
		.ucdsubname = "Extend",
	},
	{
		.enumname = "ICB_LINKER",
		.file = FILE_DCP,
		.ucdname = "InCB",
		.ucdsubname = "Linker",
	},
	{
		.enumname = "LF",
		.file = FILE_GRAPHEME,
		.ucdname = "LF",
	},
	{
		.enumname = "PREPEND",
		.file = FILE_GRAPHEME,
		.ucdname = "Prepend",
	},
	{
		.enumname = "REGIONAL_INDICATOR",
		.file = FILE_GRAPHEME,
		.ucdname = "Regional_Indicator",
	},
	{
		.enumname = "SPACINGMARK",
		.file = FILE_GRAPHEME,
		.ucdname = "SpacingMark",
	},
	{
		.enumname = "ZWJ",
		.file = FILE_GRAPHEME,
		.ucdname = "ZWJ",
	},
};

static uint_least8_t
handle_conflict(uint_least32_t cp, uint_least8_t prop1, uint_least8_t prop2)
{
	uint_least8_t result;

	(void)cp;

	if ((!strcmp(char_break_property[prop1].enumname, "EXTEND") &&
	     !strcmp(char_break_property[prop2].enumname, "ICB_EXTEND")) ||
	    (!strcmp(char_break_property[prop1].enumname, "ICB_EXTEND") &&
	     !strcmp(char_break_property[prop2].enumname, "EXTEND"))) {
		for (result = 0; result < LEN(char_break_property); result++) {
			if (!strcmp(char_break_property[result].enumname,
			            "BOTH_EXTEND_ICB_EXTEND")) {
				break;
			}
		}
		if (result == LEN(char_break_property)) {
			fprintf(stderr, "handle_conflict: Internal error.\n");
			exit(1);
		}
	} else if ((!strcmp(char_break_property[prop1].enumname, "EXTEND") &&
	            !strcmp(char_break_property[prop2].enumname,
	                    "ICB_LINKER")) ||
	           (!strcmp(char_break_property[prop1].enumname,
	                    "ICB_LINKER") &&
	            !strcmp(char_break_property[prop2].enumname, "EXTEND"))) {
		for (result = 0; result < LEN(char_break_property); result++) {
			if (!strcmp(char_break_property[result].enumname,
			            "BOTH_EXTEND_ICB_LINKER")) {
				break;
			}
		}
		if (result == LEN(char_break_property)) {
			fprintf(stderr, "handle_conflict: Internal error.\n");
			exit(1);
		}
	} else if ((!strcmp(char_break_property[prop1].enumname, "ZWJ") &&
	            !strcmp(char_break_property[prop2].enumname,
	                    "ICB_EXTEND")) ||
	           (!strcmp(char_break_property[prop1].enumname,
	                    "ICB_EXTEND") &&
	            !strcmp(char_break_property[prop2].enumname, "ZWJ"))) {
		for (result = 0; result < LEN(char_break_property); result++) {
			if (!strcmp(char_break_property[result].enumname,
			            "BOTH_ZWJ_ICB_EXTEND")) {
				break;
			}
		}
		if (result == LEN(char_break_property)) {
			fprintf(stderr, "handle_conflict: Internal error.\n");
			exit(1);
		}
	} else {
		fprintf(stderr, "handle_conflict: Cannot handle conflict.\n");
		exit(1);
	}

	return result;
}

int
main(int argc, char *argv[])
{
	(void)argc;

	properties_generate_break_property(
		char_break_property, LEN(char_break_property), NULL,
		handle_conflict, NULL, "char_break", argv[0]);

	return 0;
}
