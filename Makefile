# See LICENSE file for copyright and license details
# libgrapheme - unicode string library
.POSIX:

include config.mk

BENCHMARK =\
	benchmark/character\
	benchmark/sentence\
	benchmark/line\
	benchmark/utf8-decode\
	benchmark/word\

DATA =\
	data/EastAsianWidth.txt\
	data/emoji-data.txt\
	data/GraphemeBreakProperty.txt\
	data/GraphemeBreakTest.txt\
	data/LICENSE\
	data/LineBreak.txt\
	data/LineBreakTest.txt\
	data/SentenceBreakProperty.txt\
	data/SentenceBreakTest.txt\
	data/WordBreakProperty.txt\
	data/WordBreakTest.txt\

GEN =\
	gen/character\
	gen/character-test\
	gen/line\
	gen/line-test\
	gen/sentence\
	gen/sentence-test\
	gen/word\
	gen/word-test\

SRC =\
	src/character\
	src/line\
	src/sentence\
	src/utf8\
	src/util\
	src/word\

TEST =\
	test/character\
	test/line\
	test/sentence\
	test/utf8-decode\
	test/utf8-encode\
	test/word\

MAN3 =\
	man/grapheme_decode_utf8.3\
	man/grapheme_encode_utf8.3\
	man/grapheme_is_character_break.3\

MAN7 = man/libgrapheme.7

all: libgrapheme.a libgrapheme.so

benchmark/character.o: benchmark/character.c config.mk gen/character-test.h grapheme.h benchmark/util.h
benchmark/utf8-decode.o: benchmark/utf8-decode.c config.mk gen/character-test.h grapheme.h benchmark/util.h
benchmark/sentence.o: benchmark/sentence.c config.mk gen/sentence-test.h grapheme.h benchmark/util.h
benchmark/util.o: benchmark/util.c config.mk benchmark/util.h
benchmark/word.o: benchmark/word.c config.mk gen/word-test.h grapheme.h benchmark/util.h
gen/character.o: gen/character.c config.mk gen/util.h
gen/character-test.o: gen/character-test.c config.mk gen/util.h
gen/line.o: gen/line.c config.mk gen/util.h
gen/line-test.o: gen/line-test.c config.mk gen/util.h
gen/sentence.o: gen/sentence.c config.mk gen/util.h
gen/sentence-test.o: gen/sentence-test.c config.mk gen/util.h
gen/word.o: gen/word.c config.mk gen/util.h
gen/word-test.o: gen/word-test.c config.mk gen/util.h
gen/util.o: gen/util.c config.mk gen/util.h
src/character.o: src/character.c config.mk gen/character.h grapheme.h src/util.h
src/line.o: src/line.c config.mk gen/line.h grapheme.h src/util.h
src/sentence.o: src/sentence.c config.mk gen/sentence.h grapheme.h src/util.h
src/utf8.o: src/utf8.c config.mk grapheme.h
src/util.o: src/util.c config.mk gen/types.h grapheme.h src/util.h
src/word.o: src/word.c config.mk gen/word.h grapheme.h src/util.h
test/character.o: test/character.c config.mk gen/character-test.h grapheme.h test/util.h
test/line.o: test/line.c config.mk gen/line-test.h grapheme.h test/util.h
test/sentence.o: test/sentence.c config.mk gen/sentence-test.h grapheme.h test/util.h
test/utf8-encode.o: test/utf8-encode.c config.mk grapheme.h test/util.h
test/utf8-decode.o: test/utf8-decode.c config.mk grapheme.h test/util.h
test/util.o: test/util.c config.mk test/util.h
test/word.o: test/word.c config.mk gen/word-test.h grapheme.h test/util.h

benchmark/character: benchmark/character.o benchmark/util.o libgrapheme.a
benchmark/line: benchmark/line.o benchmark/util.o libgrapheme.a
benchmark/sentence: benchmark/sentence.o benchmark/util.o libgrapheme.a
benchmark/utf8-decode: benchmark/utf8-decode.o benchmark/util.o libgrapheme.a
benchmark/word: benchmark/word.o benchmark/util.o libgrapheme.a
gen/character: gen/character.o gen/util.o
gen/character-test: gen/character-test.o gen/util.o
gen/line: gen/line.o gen/util.o
gen/line-test: gen/line-test.o gen/util.o
gen/sentence: gen/sentence.o gen/util.o
gen/sentence-test: gen/sentence-test.o gen/util.o
gen/word: gen/word.o gen/util.o
gen/word-test: gen/word-test.o gen/util.o
test/character: test/character.o test/util.o libgrapheme.a
test/line: test/line.o test/util.o libgrapheme.a
test/sentence: test/sentence.o test/util.o libgrapheme.a
test/utf8-encode: test/utf8-encode.o test/util.o libgrapheme.a
test/utf8-decode: test/utf8-decode.o test/util.o libgrapheme.a
test/word: test/word.o test/util.o libgrapheme.a

gen/character.h: data/emoji-data.txt data/GraphemeBreakProperty.txt gen/character
gen/character-test.h: data/GraphemeBreakTest.txt gen/character-test
gen/line.h: data/emoji-data.txt data/EastAsianWidth.txt data/LineBreak.txt gen/line
gen/line-test.h: data/LineBreakTest.txt gen/line-test
gen/sentence.h: data/SentenceBreakProperty.txt gen/sentence
gen/sentence-test.h: data/SentenceBreakTest.txt gen/sentence-test
gen/word.h: data/WordBreakProperty.txt gen/word
gen/word-test.h: data/WordBreakTest.txt gen/word-test

data/EastAsianWidth.txt: data/LICENSE
	wget -O $@ https://www.unicode.org/Public/14.0.0/ucd/EastAsianWidth.txt

data/emoji-data.txt: data/LICENSE
	wget -O $@ https://www.unicode.org/Public/14.0.0/ucd/emoji/emoji-data.txt

data/GraphemeBreakProperty.txt: data/LICENSE
	wget -O $@ https://www.unicode.org/Public/14.0.0/ucd/auxiliary/GraphemeBreakProperty.txt

data/GraphemeBreakTest.txt: data/LICENSE
	wget -O $@ https://www.unicode.org/Public/14.0.0/ucd/auxiliary/GraphemeBreakTest.txt

data/LICENSE:
	wget -O $@ https://www.unicode.org/license.txt

data/LineBreak.txt: data/LICENSE
	wget -O $@ https://www.unicode.org/Public/14.0.0/ucd/LineBreak.txt

data/LineBreakTest.txt: data/LICENSE
	wget -O $@ https://www.unicode.org/Public/14.0.0/ucd/auxiliary/LineBreakTest.txt

data/SentenceBreakProperty.txt: data/LICENSE
	wget -O $@ https://www.unicode.org/Public/14.0.0/ucd/auxiliary/SentenceBreakProperty.txt

data/SentenceBreakTest.txt: data/LICENSE
	wget -O $@ https://www.unicode.org/Public/14.0.0/ucd/auxiliary/SentenceBreakTest.txt

data/WordBreakProperty.txt: data/LICENSE
	wget -O $@ https://www.unicode.org/Public/14.0.0/ucd/auxiliary/WordBreakProperty.txt

data/WordBreakTest.txt: data/LICENSE
	wget -O $@ https://www.unicode.org/Public/14.0.0/ucd/auxiliary/WordBreakTest.txt

$(BENCHMARK):
	$(CC) -o $@ $(LDFLAGS) $@.o benchmark/util.o libgrapheme.a -lutf8proc

$(GEN):
	$(CC) -o $@ $(LDFLAGS) $@.o gen/util.o

$(GEN:=.h):
	$(@:.h=) > $@

$(TEST):
	$(CC) -o $@ $(LDFLAGS) $@.o test/util.o libgrapheme.a

.c.o:
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

libgrapheme.a: $(SRC:=.o)
	$(AR) rc $@ $?
	$(RANLIB) $@

libgrapheme.so: $(SRC:=.o)
	$(CC) -o $@ -shared $?

benchmark: $(BENCHMARK)
	for m in $(BENCHMARK); do ./$$m; done

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
	ldconfig || true

uninstall:
	for m in $(MAN3); do rm -f "$(DESTDIR)$(MANPREFIX)/man3/`basename $$m`"; done
	for m in $(MAN7); do rm -f "$(DESTDIR)$(MANPREFIX)/man7/`basename $$m`"; done
	rm -f "$(DESTDIR)$(LIBPREFIX)/libgrapheme.a"
	rm -f "$(DESTDIR)$(LIBPREFIX)/libgrapheme.so"
	rm -f "$(DESTDIR)$(INCPREFIX)/grapheme.h"
	ldconfig || true

clean:
	rm -f $(BENCHMARK:=.o) benchmark/util.o $(BENCHMARK) $(GEN:=.h) $(GEN:=.o) gen/util.o $(GEN) $(SRC:=.o) src/util.o $(TEST:=.o) test/util.o $(TEST) libgrapheme.a libgrapheme.so

clean-data:
	rm -f $(DATA)

print:
	@echo $(PREFIX)

dist:
	rm -rf "libgrapheme-$(VERSION)"
	mkdir "libgrapheme-$(VERSION)"
	for m in benchmark data gen man src test; do mkdir "libgrapheme-$(VERSION)/$$m"; done
	cp config.mk grapheme.h LICENSE Makefile README "libgrapheme-$(VERSION)"
	cp $(BENCHMARK:=.c) benchmark/util.c benchmark/util.h "libgrapheme-$(VERSION)/benchmark"
	cp $(DATA) "libgrapheme-$(VERSION)/data"
	cp $(GEN:=.c) gen/util.c gen/types.h gen/util.h "libgrapheme-$(VERSION)/gen"
	cp $(MAN3) $(MAN7) "libgrapheme-$(VERSION)/man"
	cp $(SRC:=.c) src/util.h "libgrapheme-$(VERSION)/src"
	cp $(TEST:=.c) test/util.c test/util.h "libgrapheme-$(VERSION)/test"
	tar -cf - "libgrapheme-$(VERSION)" | gzip -c > "libgrapheme-$(VERSION).tar.gz"
	rm -rf "libgrapheme-$(VERSION)"

.PHONY: all benchmark test install uninstall clean clean-data dist
