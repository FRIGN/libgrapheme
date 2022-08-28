cat << EOF
.Dd $MAN_DATE
.Dt GRAPHEME_TO_$(printf $CASE | tr [:lower:] [:upper:]) 3
.Os suckless.org
.Sh NAME
.Nm grapheme_to_$CASE
.Nd convert codepoint array to $CASE
.Sh SYNOPSIS
.In grapheme.h
.Ft size_t
.Fn grapheme_to_$CASE "const uint_least32_t *src" "size_t srclen" "uint_least32_t *dest" "size_t destlen"
.Sh DESCRIPTION
The
.Fn grapheme_to_$CASE
function converts the codepoint array
.Va str
to $CASE and writes the result to
.Va dest
up to
.Va destlen ,
unless
.Va dest
is set to
.Dv NULL .
.Pp
If
.Va len
is set to
.Dv SIZE_MAX
(stdint.h is already included by grapheme.h) the string
.Va src
is interpreted to be NUL-terminated and processing stops when a
NUL-byte is encountered.
.Pp
For UTF-8-encoded input data
.Xr grapheme_to_$CASE_utf8 3
can be used instead.
.Sh RETURN VALUES
The
.Fn grapheme_to_$CASE
function returns the number of codepoints in the array resulting
from converting
.Va src
to $CASE, even if
.Va len
is not large enough or
.Va dest
is
.Dv NULL .
.Sh SEE ALSO
.Xr grapheme_to_$CASE_utf8 3 ,
.Xr libgrapheme 7
.Sh STANDARDS
.Fn grapheme_to_$CASE
is compliant with the Unicode $UNICODE_VERSION specification.
.Sh AUTHORS
.An Laslo Hunhold Aq Mt dev@frign.de
EOF
