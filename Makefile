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

clean:
	rm -f anagram is_spellable spellable *.exe *.o
