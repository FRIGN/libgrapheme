/* See LICENSE file for copyright and license details. */
#include <stddef.h>

#include "util.h"

#define FILE_BIDICLASS "data/DerivedBidiClass.txt"

static const struct property_spec bidi_property[] = {
	{
		/* default */
		.enumname = "L",
		.file     = FILE_BIDICLASS,
		.ucdname  = "L",
	},
	{
		.enumname = "AL",
		.file     = FILE_BIDICLASS,
		.ucdname  = "AL",
	},
	{
		.enumname = "AN",
		.file     = FILE_BIDICLASS,
		.ucdname  = "AN",
	},
	{
		.enumname = "B",
		.file     = FILE_BIDICLASS,
		.ucdname  = "B",
	},
	{
		.enumname = "BN",
		.file     = FILE_BIDICLASS,
		.ucdname  = "BN",
	},
	{
		.enumname = "CS",
		.file     = FILE_BIDICLASS,
		.ucdname  = "CS",
	},
	{
		.enumname = "EN",
		.file     = FILE_BIDICLASS,
		.ucdname  = "EN",
	},
	{
		.enumname = "ES",
		.file     = FILE_BIDICLASS,
		.ucdname  = "Es",
	},
	{
		.enumname = "ET",
		.file     = FILE_BIDICLASS,
		.ucdname  = "ET",
	},
	{
		.enumname = "FSI",
		.file     = FILE_BIDICLASS,
		.ucdname  = "FSI",
	},
	{
		.enumname = "LRE",
		.file     = FILE_BIDICLASS,
		.ucdname  = "LRE",
	},
	{
		.enumname = "LRI",
		.file     = FILE_BIDICLASS,
		.ucdname  = "LRI",
	},
	{
		.enumname = "LRO",
		.file     = FILE_BIDICLASS,
		.ucdname  = "LRO",
	},
	{
		.enumname = "NSM",
		.file     = FILE_BIDICLASS,
		.ucdname  = "NSM",
	},
	{
		.enumname = "ON",
		.file     = FILE_BIDICLASS,
		.ucdname  = "ON",
	},
	{
		.enumname = "PDF",
		.file     = FILE_BIDICLASS,
		.ucdname  = "PDF",
	},
	{
		.enumname = "PDI",
		.file     = FILE_BIDICLASS,
		.ucdname  = "PDI",
	},
	{
		.enumname = "R",
		.file     = FILE_BIDICLASS,
		.ucdname  = "R",
	},
	{
		.enumname = "RLE",
		.file     = FILE_BIDICLASS,
		.ucdname  = "RLE",
	},
	{
		.enumname = "RLI",
		.file     = FILE_BIDICLASS,
		.ucdname  = "RLI",
	},
	{
		.enumname = "RLO",
		.file     = FILE_BIDICLASS,
		.ucdname  = "RLO",
	},
	{
		.enumname = "S",
		.file     = FILE_BIDICLASS,
		.ucdname  = "S",
	},
	{
		.enumname = "WS",
		.file     = FILE_BIDICLASS,
		.ucdname  = "WS",
	},
};

int
main(int argc, char *argv[])
{
	(void)argc;

	properties_generate_break_property(bidi_property,
	                                   LEN(bidi_property),
	                                   NULL, NULL, "bidi", argv[0]);

	return 0;
}
