# See LICENSE file for copyright and license details.

# http://www.unicode.org/Public/UCD/latest/ucd/auxiliary/GraphemeBreakProperty.txt
BEGIN {
	FS = "[ ;]+"
}

$0 ~ /^#/ || $0 ~ /^\s*$/  { next }
$2 == "CR"                 { crs[ncrs++] = $1 }
$2 == "LF"                 { lfs[nlfs++] = $1 }
$2 == "Control"            { controls[ncontrols++] = $1 }
$2 == "Extend"             { extends[nextends++] = $1 }
$2 == "ZWJ"                { zwj[nzwj++] = $1 }
$2 == "Regional_Indicator" { ris[nris++] = $1 }
$2 == "Prepend"            { prepends[nprepends++] = $1 }
$2 == "SpacingMark"        { spacingmarks[nspacingmarks++] = $1 }
$2 == "L"                  { ls[nls++] = $1 }
$2 == "V"                  { vs[nvs++] = $1 }
$2 == "T"                  { ts[nts++] = $1 }
$2 == "LV"                 { lvs[nlvs++] = $1 }
$2 == "LVT"                { lvts[nlvts++] = $1 }

END {
	mktable("cr", crs, ncrs);
	mktable("lf", lfs, nlfs);
	mktable("control", controls, ncontrols);
	mktable("extend", extends, nextends);
	mktable("zwj", zwj, nzwj);
	mktable("ri", ris, nris);
	mktable("prepend", prepends, nprepends);
	mktable("spacingmark", spacingmarks, nspacingmarks);
	mktable("l", ls, nls);
	mktable("v", vs, nvs);
	mktable("t", ts, nts);
	mktable("lv", lvs, nlvs);
	mktable("lvt", lvts, nlvts);
}

function hextonum(str) {
	str = tolower(str);
	if (substr(str, 1, 2) != "0x") {
		return -1;
	}
	str = substr(str, 3);

	val = 0;
	for (i = 0; i < length(str); i++) {
		dig = index("0123456789abcdef", substr(str, i + 1, 1));

		if (!dig) {
			return -1;
		}

		val = (16 * val) + (dig - 1);
	}

	return val;
}

function mktable(name, array, arrlen) {
	printf("static const uint32_t "name"_table[][2] = {\n");

	for (j = 0; j < arrlen; j++) {
		if (ind = index(array[j], "..")) {
			lower = tolower(substr(array[j], 1, ind - 1));
			upper = tolower(substr(array[j], ind + 2));
		} else {
			lower = upper = tolower(array[j]);
		}
		lower = sprintf("0x%s", lower);
		upper = sprintf("0x%s", upper);

		# print lower bound
		printf("\t{ %s, ", lower);

		for (; j < arrlen - 1; j++) {
			# look ahead and check if we have adjacent arrays
			if (ind = index(array[j + 1], "..")) {
				nextlower = tolower(substr(array[j + 1],
				                    1, ind - 1));
				nextupper = tolower(substr(array[j + 1],
				                    ind + 2));
			} else {
				nextlower = nextupper = tolower(array[j + 1]);
			}
			nextlower = sprintf("0x%s", nextlower);
			nextupper = sprintf("0x%s", nextupper);

			if ((hextonum(nextlower) * 1) != (hextonum(upper) + 1)) {
				break;
			} else {
				upper = nextupper;
			}
		}

		# print upper bound
		printf("%s },\n", upper);
	}

	printf("};\n");
}
