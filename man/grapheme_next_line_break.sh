cat << EOF
.Dd 2022-08-26
.Dt GRAPHEME_NEXT_LINE_BREAK 3
.Os suckless.org
.Sh NAME
.Nm grapheme_next_line_break
.Nd determine codepoint-offset to next grapheme cluster break
.Sh SYNOPSIS
.In grapheme.h
.Ft size_t
.Fn grapheme_next_line_break "const uint_least32_t *str" "size_t len"
.Sh DESCRIPTION
The
.Fn grapheme_next_line_break
function computes the offset (in codepoints) to the next possible line
break (see
.Xr libgrapheme 7 )
in the codepoint array
.Va str
of length
.Va len .
.Pp
If
.Va len
is set to
.Dv SIZE_MAX
(stdint.h is already included by grapheme.h) the string
.Va str
is interpreted to be NUL-terminated and processing stops when a
NUL-byte is encountered.
.Pp
For UTF-8-encoded input data
.Xr grapheme_next_line_break_utf8 3
can be used instead.
.Sh RETURN VALUES
The
.Fn grapheme_next_line_break
function returns the offset (in codepoints) to the next possible line
break in
.Va str
or 0 if
.Va str
is
.Dv NULL .
.Sh SEE ALSO
.Xr grapheme_next_line_break_utf8 3 ,
.Xr libgrapheme 7
.Sh STANDARDS
.Fn grapheme_next_line_break
is compliant with the Unicode 14.0.0 specification.
.Sh AUTHORS
.An Laslo Hunhold Aq Mt dev@frign.de
EOF
