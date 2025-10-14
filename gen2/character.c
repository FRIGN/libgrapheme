/* See LICENSE file for copyright and license details. */
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "character.h"
#include "util.h"

int
main(int argc, char *argv[])
{
	const struct codepoint_property *match;
	struct codepoint_property_set *properties_dcp, *properties_emoji,
	                              *properties_grapheme;
	uint_least64_t *properties;
	uint_least32_t cp;

	(void)argc;
	
	/* parse properties from the Unicode data files */
	properties_dcp = parse_property_file("data/DerivedCoreProperties.txt");
	properties_emoji = parse_property_file("data/emoji-data.txt");
	properties_grapheme = parse_property_file("data/GraphemeBreakProperty.txt");

	/* allocate property array and initialise to zero */
	if (!(properties = calloc(UINT32_C(0x110000), sizeof(*properties)))) {	
		fprintf(stderr, "%s: malloc: %s\n", argv[0], strerror(errno));
		exit(1);
	}

	for (cp = 0; cp <= UINT32_C(0x10FFFF); cp++) {
		if (match_in_codepoint_property_set(
			&(properties_grapheme[cp]), "Control", 0)) {
			properties[cp] |= CHAR_PROP_CONTROL;
		}

		if (match_in_codepoint_property_set(
			&(properties_grapheme[cp]), "Extend", 0)) {
			properties[cp] |= CHAR_PROP_EXTEND;
		}

		if (match_in_codepoint_property_set(
			&(properties_emoji[cp]), "Extended_Pictographic", 0)) {
			properties[cp] |= CHAR_PROP_EXTENDED_PICTOGRAPHIC;
		}

		if (match_in_codepoint_property_set(
			&(properties_grapheme[cp]), "L", 0)) {
			properties[cp] |= CHAR_PROP_HANGUL_L;
		}

		if (match_in_codepoint_property_set(
			&(properties_grapheme[cp]), "V", 0)) {
			properties[cp] |= CHAR_PROP_HANGUL_V;
		}

		if (match_in_codepoint_property_set(
			&(properties_grapheme[cp]), "T", 0)) {
			properties[cp] |= CHAR_PROP_HANGUL_T;
		}

		if (match_in_codepoint_property_set(
			&(properties_grapheme[cp]), "LV", 0)) {
			properties[cp] |= CHAR_PROP_HANGUL_LV;
		}

		if (match_in_codepoint_property_set(
			&(properties_grapheme[cp]), "LVT", 0)) {
			properties[cp] |= CHAR_PROP_HANGUL_LVT;
		}

		if ((match = match_in_codepoint_property_set(
			&(properties_dcp[cp]), "InCB", 0))) {
			if (strcmp(match->fields[1], "Consonant") == 0) {
				properties[cp] |= CHAR_PROP_ICB_CONSONANT;
			} else if (strcmp(match->fields[1], "Extend") == 0) {
				properties[cp] |= CHAR_PROP_ICB_EXTEND;
			} else if (strcmp(match->fields[1], "Linker") == 0) {
				properties[cp] |= CHAR_PROP_ICB_LINKER;
			}
		}

		if (match_in_codepoint_property_set(
			&(properties_grapheme[cp]), "Prepend", 0)) {
			properties[cp] |= CHAR_PROP_PREPEND;
		}

		if (match_in_codepoint_property_set(
			&(properties_grapheme[cp]), "Regional_Indicator", 0)) {
			properties[cp] |= CHAR_PROP_REGIONAL_INDICATOR;
		}

		if (match_in_codepoint_property_set(
			&(properties_grapheme[cp]), "SpacingMark", 0)) {
			properties[cp] |= CHAR_PROP_SPACINGMARK;
		}
	}

	/* generate code */
	compress_and_output(properties, "character");

	/* cleanup */
	free_codepoint_property_set_array(properties_dcp);
	free_codepoint_property_set_array(properties_emoji);
	free_codepoint_property_set_array(properties_grapheme);
	free(properties);

	return 0;
}
