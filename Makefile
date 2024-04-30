CC ?= gcc
CFLAGS ?= -O3 -fPIC -Wall -Werror
LDFLAGS ?=

# Enable threading
CFLAGS += -DENABLE_THREADING -pthread
LDFLAGS += -pthread

DEFAULT = anagram is_spellable spellable qwantzle
all: anagram is_spellable spellable qwantzle

anagram: anagram.o letter_pool.o phrase_list.o sentence.o
	$(CC) -o $@ $+ $(LDFLAGS)

anagwin: anagwin.o letter_pool.o phrase_list.o sentence.o
	$(CC) -mwindows -o $@ $+ $(LDFLAGS)

is_spellable: is_spellable.o letter_pool.o
	$(CC) -o $@ $+ $(LDFLAGS)

spellable: spellable.o letter_pool.o phrase_list.o
	$(CC) -o $@ $+ $(LDFLAGS)

qwantzle: qwantzle.o letter_pool.o phrase_list.o sentence.o
	$(CC) -o $@ $+ $(LDFLAGS)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f anagram is_spellable spellable qwantzle *.exe *.o
