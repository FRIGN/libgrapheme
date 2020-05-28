# See LICENSE file for copyright and license details.

# https://www.unicode.org/Public/UCD/latest/ucd/auxiliary/GraphemeBreakTest.txt
BEGIN {
	FS = " "

	printf("struct test {\n\tCodepoint *cp;\n\tsize_t cplen;\n");
	printf("\tsize_t *len;\n\tsize_t lenlen;\n\tchar *descr;\n};\n\n");
	printf("static const struct test t[] = {\n");
}

$0 ~ /^#/ || $0 ~ /^\s*$/ { next }

{
	ncps = 0;
	nlens = 0;

	curlen = 1;
	for (i = 2; i <= NF; i++) {
		if ($(i + 1) == "#") {
			break;
		}
		if (i % 2 == 0) {
			# code point
			cp[ncps++] = tolower($i);
		} else {
			# break information
			if ($i == "÷") {
				# break
				len[nlens++] = curlen;
				curlen = 1;
			} else { # $i == "×"
				# no break
				curlen++;
			}
		}
	}
	len[nlens++] = curlen;

	# print code points
	printf("\t{\n\t\t.cp     = (Codepoint[]){ ");
	for (i = 0; i < ncps; i++) {
		printf("0x%s", cp[i]);
		if (i + 1 < ncps) {
			printf(", ");
		}
	}
	printf(" },\n\t\t.cplen  = %d,\n", ncps);

	# print grapheme cluster lengths
	printf("\t\t.len    = (size_t[]){ ");
	for (i = 0; i < nlens; i++) {
		printf("%s", len[i]);
		if (i + 1 < nlens) {
			printf(", ");
		}
	}
	printf(" },\n\t\t.lenlen = %d,\n", nlens);

	# print testcase description
	printf("\t\t.descr  = \"%s\",\n", substr($0, index($0, "#") + 3));

	printf("\t},\n");
}

END {
	printf("};\n");
}
