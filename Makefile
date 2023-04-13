CC ?= gcc
CFLAGS ?= -O3 -fPIC -Wall -Werror
LDFLAGS ?=

DEFAULT = anagram is_spellable spellable
all: anagram is_spellable spellable

anagram: anagram.o letter_pool.o phrase_list.o sentence.o
	$(CC) -o $@ $+ $(LDFLAGS)

is_spellable: is_spellable.o letter_pool.o
	$(CC) -o $@ $+ $(LDFLAGS)

spellable: spellable.o letter_pool.o phrase_list.o
	$(CC) -o $@ $+ $(LDFLAGS)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

# Test suite -- not intended for general consumption
test: letter_pool
	./letter_pool
letter_pool: letter_pool.c
	$(CC) $(CFLAGS) -DTEST_LETTER_POOL -o $@ $+ $(LDFLAGS)

clean:
	rm -f anagram is_spellable letter_pool spellable *.exe *.o
