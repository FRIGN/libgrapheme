/* See LICENSE file for copyright and license details. */
#include <stddef.h>

#include "util.h"

#define FILE_BIDI_CLASS     "data/DerivedBidiClass.txt"
#define FILE_BIDI_MIRRORING "data/BidiMirroring.txt"

static const struct property_spec bidi_property[] = {
	{
		/* default */
		.enumname = "L",
		.file     = FILE_BIDI_CLASS,
		.ucdname  = "L",
	},
	{
		.enumname = "AL",
		.file     = FILE_BIDI_CLASS,
		.ucdname  = "AL",
	},
	{
		.enumname = "AN",
		.file     = FILE_BIDI_CLASS,
		.ucdname  = "AN",
	},
	{
		.enumname = "B",
		.file     = FILE_BIDI_CLASS,
		.ucdname  = "B",
	},
	{
		.enumname = "BN",
		.file     = FILE_BIDI_CLASS,
		.ucdname  = "BN",
	},
	{
		.enumname = "CS",
		.file     = FILE_BIDI_CLASS,
		.ucdname  = "CS",
	},
	{
		.enumname = "EN",
		.file     = FILE_BIDI_CLASS,
		.ucdname  = "EN",
	},
	{
		.enumname = "ES",
		.file     = FILE_BIDI_CLASS,
		.ucdname  = "ES",
	},
	{
		.enumname = "ET",
		.file     = FILE_BIDI_CLASS,
		.ucdname  = "ET",
	},
	{
		.enumname = "FSI",
		.file     = FILE_BIDI_CLASS,
		.ucdname  = "FSI",
	},
	{
		.enumname = "LRE",
		.file     = FILE_BIDI_CLASS,
		.ucdname  = "LRE",
	},
	{
		.enumname = "LRI",
		.file     = FILE_BIDI_CLASS,
		.ucdname  = "LRI",
	},
	{
		.enumname = "LRO",
		.file     = FILE_BIDI_CLASS,
		.ucdname  = "LRO",
	},
	{
		.enumname = "NSM",
		.file     = FILE_BIDI_CLASS,
		.ucdname  = "NSM",
	},
	{
		.enumname = "ON",
		.file     = FILE_BIDI_CLASS,
		.ucdname  = "ON",
	},
	{
		.enumname = "PDF",
		.file     = FILE_BIDI_CLASS,
		.ucdname  = "PDF",
	},
	{
		.enumname = "PDI",
		.file     = FILE_BIDI_CLASS,
		.ucdname  = "PDI",
	},
	{
		.enumname = "R",
		.file     = FILE_BIDI_CLASS,
		.ucdname  = "R",
	},
	{
		.enumname = "RLE",
		.file     = FILE_BIDI_CLASS,
		.ucdname  = "RLE",
	},
	{
		.enumname = "RLI",
		.file     = FILE_BIDI_CLASS,
		.ucdname  = "RLI",
	},
	{
		.enumname = "RLO",
		.file     = FILE_BIDI_CLASS,
		.ucdname  = "RLO",
	},
	{
		.enumname = "S",
		.file     = FILE_BIDI_CLASS,
		.ucdname  = "S",
	},
	{
		.enumname = "WS",
		.file     = FILE_BIDI_CLASS,
		.ucdname  = "WS",
	},
};

static uint_least8_t
fill_missing(uint_least32_t cp) {
	/* based on the @missing-properties in data/DerivedBidiClass.txt */
	if ((cp >= UINT32_C(0x0590)  && cp <= UINT32_C(0x05FF))  ||
	    (cp >= UINT32_C(0x07C0)  && cp <= UINT32_C(0x085F))  ||
	    (cp >= UINT32_C(0xFB1D)  && cp <= UINT32_C(0xFB4F))  ||
	    (cp >= UINT32_C(0x10800) && cp <= UINT32_C(0x10CFF)) ||
	    (cp >= UINT32_C(0x10D40) && cp <= UINT32_C(0x10EBF)) ||
	    (cp >= UINT32_C(0x10F00) && cp <= UINT32_C(0x10F2F)) ||
	    (cp >= UINT32_C(0x10F70) && cp <= UINT32_C(0x10FFF)) ||
	    (cp >= UINT32_C(0x1E800) && cp <= UINT32_C(0x1EC6F)) ||
	    (cp >= UINT32_C(0x1ECC0) && cp <= UINT32_C(0x1ECFF)) ||
	    (cp >= UINT32_C(0x1ED50) && cp <= UINT32_C(0x1EDFF)) ||
	    (cp >= UINT32_C(0x1EF00) && cp <= UINT32_C(0x1EFFF))) {
		return 17; /* class R */
	} else if ((cp >= UINT32_C(0x0600)  && cp <= UINT32_C(0x07BF))  ||
	           (cp >= UINT32_C(0x0860)  && cp <= UINT32_C(0x08FF))  ||
	           (cp >= UINT32_C(0xFB50)  && cp <= UINT32_C(0xFDCF))  ||
	           (cp >= UINT32_C(0xFDF0)  && cp <= UINT32_C(0xFDFF))  ||
	           (cp >= UINT32_C(0xFE70)  && cp <= UINT32_C(0xFEFF))  ||
	           (cp >= UINT32_C(0x10D00) && cp <= UINT32_C(0x10D3F)) ||
	           (cp >= UINT32_C(0x10EC0) && cp <= UINT32_C(0x10EFF)) ||
		   (cp >= UINT32_C(0x10F30) && cp <= UINT32_C(0x10F6F)) ||
	           (cp >= UINT32_C(0x1EC70) && cp <= UINT32_C(0x1ECBF)) ||
	           (cp >= UINT32_C(0x1ED00) && cp <= UINT32_C(0x1ED4F)) ||
	           (cp >= UINT32_C(0x1EE00) && cp <= UINT32_C(0x1EEFF))) {
		return 1;  /* class AL */
	} else if (cp >= UINT32_C(0x20A0) && cp <= UINT32_C(0x20CF)) {
		return 8;  /* class ET */
	} else {
		return 0;  /* class L */
	}
}

int
main(int argc, char *argv[])
{
	(void)argc;

	properties_generate_break_property(bidi_property,
	                                   LEN(bidi_property), fill_missing,
	                                   NULL, NULL, "bidi", argv[0]);

	return 0;
}
