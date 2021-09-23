# See LICENSE file for copyright and license details
# libgrapheme - grapheme cluster utility library
.POSIX:

include config.mk

LIB = src/boundary src/codepoint src/grapheme src/util
TEST = test/grapheme_boundary test/utf8-decode test/utf8-encode
DATA = data/emoji data/grapheme_boundary data/grapheme_boundary_test

MAN3 = man/grapheme_bytelen.3
MAN7 = man/libgrapheme.7

all: libgrapheme.a libgrapheme.so

data/emoji.h: data/emoji.txt data/emoji
data/grapheme_boundary.h: data/grapheme_boundary.txt data/grapheme_boundary
data/grapheme_boundary_test.h: data/grapheme_boundary_test.txt data/grapheme_boundary_test

data/emoji.o: data/emoji.c config.mk data/datautil.h
data/grapheme_boundary.o: data/grapheme_boundary.c config.mk data/datautil.h
data/grapheme_boundary_test.o: data/grapheme_boundary_test.c config.mk data/datautil.h
data/datautil.o: data/datautil.c config.mk data/datautil.h
src/boundary.o: src/boundary.c config.mk data/emoji.h data/grapheme_boundary.h grapheme.h
src/codepoint.o: src/codepoint.c config.mk grapheme.h
src/grapheme.o: src/grapheme.c config.mk grapheme.h
src/util.o: src/util.c config.mk src/util.h
test/grapheme_boundary.o: test/grapheme_boundary.c config.mk data/grapheme_boundary_test.h grapheme.h
test/utf8-encode.o: test/utf8-encode.c config.mk grapheme.h
test/utf8-decode.o: test/utf8-decode.c config.mk grapheme.h

data/emoji: data/emoji.o data/datautil.o
data/grapheme_boundary: data/grapheme_boundary.o data/datautil.o
data/grapheme_boundary_test: data/grapheme_boundary_test.o data/datautil.o
test/grapheme_boundary: test/grapheme_boundary.o libgrapheme.a
test/utf8-encode: test/utf8-encode.o libgrapheme.a
test/utf8-decode: test/utf8-decode.o libgrapheme.a

data/emoji.txt:
	wget -O $@ https://www.unicode.org/Public/14.0.0/ucd/emoji/emoji-data.txt

data/grapheme_boundary.txt:
	wget -O $@ https://www.unicode.org/Public/14.0.0/ucd/auxiliary/GraphemeBreakProperty.txt

data/grapheme_boundary_test.txt:
	wget -O $@ https://www.unicode.org/Public/14.0.0/ucd/auxiliary/GraphemeBreakTest.txt

$(DATA:=.h):
	$(@:.h=) < $(@:.h=.txt) > $@

$(DATA):
	$(CC) -o $@ $(LDFLAGS) $@.o data/datautil.o

$(TEST):
	$(CC) -o $@ $(LDFLAGS) $@.o libgrapheme.a

.c.o:
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

libgrapheme.a: $(LIB:=.o)
	$(AR) rc $@ $?
	$(RANLIB) $@

libgrapheme.so: $(LIB:=.o)
	$(CC) -o $@ -shared $?

test: $(TEST)
	for m in $(TEST); do ./$$m; done

install: all
	mkdir -p "$(DESTDIR)$(LIBPREFIX)"
	mkdir -p "$(DESTDIR)$(INCPREFIX)"
	mkdir -p "$(DESTDIR)$(MANPREFIX)/man3"
	mkdir -p "$(DESTDIR)$(MANPREFIX)/man7"
	cp -f $(MAN3) "$(DESTDIR)$(MANPREFIX)/man3"
	cp -f $(MAN7) "$(DESTDIR)$(MANPREFIX)/man7"
	cp -f libgrapheme.a "$(DESTDIR)$(LIBPREFIX)"
	cp -f libgrapheme.so "$(DESTDIR)$(LIBPREFIX)"
	cp -f grapheme.h "$(DESTDIR)$(INCPREFIX)"

uninstall:
	for m in $(MAN3); do rm -f "$(DESTDIR)$(MANPREFIX)/man3/`basename $$m`"; done
	for m in $(MAN7); do rm -f "$(DESTDIR)$(MANPREFIX)/man7/`basename $$m`"; done
	rm -f "$(DESTDIR)$(LIBPREFIX)/libgrapheme.a"
	rm -f "$(DESTDIR)$(LIBPREFIX)/libgrapheme.so"
	rm -f "$(DESTDIR)$(INCPREFIX)/grapheme.h"

clean:
	rm -f $(DATA:=.h) $(DATA:=.o) data/datautil.o $(LIB:=.o) $(TEST:=.o) $(DATA) $(TEST) libgrapheme.a libgrapheme.so

clean-data:
	rm -f $(DATA:=.txt)
