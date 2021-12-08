# See LICENSE file for copyright and license details
# libgrapheme - grapheme cluster utility library
.POSIX:

include config.mk

DATA =\
	data/emoji-data.txt\
	data/GraphemeBreakProperty.txt\
	data/GraphemeBreakTest.txt
GEN = gen/grapheme gen/grapheme-test
LIB = src/codepoint src/grapheme src/util
TEST = test/grapheme test/utf8-decode test/utf8-encode

MAN3 = man/grapheme_bytelen.3
MAN7 = man/libgrapheme.7

all: libgrapheme.a libgrapheme.so

gen/grapheme.o: gen/grapheme.c config.mk gen/util.h
gen/grapheme-test.o: gen/grapheme-test.c config.mk gen/util.h
gen/util.o: gen/util.c config.mk gen/util.h
src/codepoint.o: src/codepoint.c config.mk grapheme.h
src/grapheme.o: src/grapheme.c config.mk gen/grapheme.h grapheme.h src/util.h
src/util.o: src/util.c config.mk src/util.h
test/grapheme.o: test/grapheme.c config.mk gen/grapheme-test.h grapheme.h
test/utf8-encode.o: test/utf8-encode.c config.mk grapheme.h
test/utf8-decode.o: test/utf8-decode.c config.mk grapheme.h

gen/grapheme: gen/grapheme.o gen/util.o
gen/grapheme-test: gen/grapheme-test.o gen/util.o
test/grapheme: test/grapheme.o libgrapheme.a
test/utf8-encode: test/utf8-encode.o libgrapheme.a
test/utf8-decode: test/utf8-decode.o libgrapheme.a

gen/grapheme.h: data/emoji-data.txt data/GraphemeBreakProperty.txt gen/grapheme
gen/grapheme-test.h: data/GraphemeBreakTest.txt gen/grapheme-test

data/emoji-data.txt:
	wget -O $@ https://www.unicode.org/Public/14.0.0/ucd/emoji/emoji-data.txt

data/GraphemeBreakProperty.txt:
	wget -O $@ https://www.unicode.org/Public/14.0.0/ucd/auxiliary/GraphemeBreakProperty.txt

data/GraphemeBreakTest.txt:
	wget -O $@ https://www.unicode.org/Public/14.0.0/ucd/auxiliary/GraphemeBreakTest.txt

$(GEN):
	$(CC) -o $@ $(LDFLAGS) $@.o gen/util.o

$(GEN:=.h):
	$(@:.h=) > $@

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
	rm -f $(GEN:=.h) $(GEN:=.o) $(GEN) gen/util.o $(LIB:=.o) $(TEST:=.o) $(TEST) libgrapheme.a libgrapheme.so

clean-data:
	rm -f $(DATA)
