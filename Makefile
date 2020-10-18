# See LICENSE file for copyright and license details
# libgrapheme - grapheme cluster utility library
.POSIX:

include config.mk

LIB = src/boundary src/codepoint src/grapheme
TEST = test/grapheme_break test/utf8-decode test/utf8-encode
DATA = data/gbp data/emo data/gbt

MAN3 = man/grapheme_bytelen.3
MAN7 = man/libgrapheme.7

all: libgrapheme.a libgrapheme.so $(TEST)

data/gbp.h: data/gbp.txt data/gbp
data/emo.h: data/emo.txt data/emo
data/gbt.h: data/gbt.txt data/gbt

data/gbp.o: data/gbp.c config.mk data/util.h
data/emo.o: data/emo.c config.mk data/util.h
data/gbt.o: data/gbt.c config.mk data/util.h
data/util.o: data/util.c config.mk data/util.h
src/boundary.o: src/boundary.c config.mk data/emo.h data/gbp.h grapheme.h
src/codepoint.o: src/codepoint.c config.mk grapheme.h
src/grapheme.o: src/grapheme.c config.mk grapheme.h
test/grapheme_break.o: test/grapheme_break.c config.mk data/gbt.h grapheme.h
test/utf8-encode.o: test/utf8-encode.c config.mk grapheme.h
test/utf8-decode.o: test/utf8-decode.c config.mk grapheme.h

data/gbp: data/gbp.o data/util.o
data/emo: data/emo.o data/util.o
data/gbt: data/gbt.o data/util.o
test/grapheme_break: test/grapheme_break.o $(LIB:=.o)
test/utf8-encode: test/utf8-encode.o $(LIB:=.o)
test/utf8-decode: test/utf8-decode.o $(LIB:=.o)

data/gbp.txt:
	wget -O $@ https://www.unicode.org/Public/13.0.0/ucd/auxiliary/GraphemeBreakProperty.txt

data/emo.txt:
	wget -O $@ https://www.unicode.org/Public/13.0.0/ucd/emoji/emoji-data.txt

data/gbt.txt:
	wget -O $@ https://www.unicode.org/Public/13.0.0/ucd/auxiliary/GraphemeBreakTest.txt

$(DATA:=.h):
	$(@:.h=) < $(@:.h=.txt) > $@

$(DATA):
	$(CC) -o $@ $(LDFLAGS) $@.o data/util.o

$(TEST):
	$(CC) -o $@ $(LDFLAGS) $@.o $(LIB:=.o)

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
	rm -f $(DATA:=.h) $(DATA:=.o) data/util.o $(LIB:=.o) $(TEST:=.o) $(DATA) $(TEST) libgrapheme.a libgrapheme.so

clean-data:
	rm -f $(DATA:=.txt)
