CC ?= gcc
CFLAGS ?= -O3 -fPIC -Wall -Werror
LDFLAGS ?=

DEFAULT = anagram_search is_spellable word_search
all: anagram_search is_spellable word_search

anagram_search: anagram_search.o letter_pool.o util.o
	$(CC) -o $@ $+ $(LDFLAGS)

is_spellable: is_spellable.o letter_pool.o
	$(CC) -o $@ $+ $(LDFLAGS)

word_search: word_search.o letter_pool.o util.o
	$(CC) -o $@ $+ $(LDFLAGS)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

# Test suite -- not intended for general consumption
test: letter_pool
	./letter_pool
letter_pool: letter_pool.c
	$(CC) $(CFLAGS) -DTEST_LETTER_POOL -o $@ $+ $(LDFLAGS)

clean:
	rm -f anagram_search is_spellable letter_pool word_search *.exe *.o
