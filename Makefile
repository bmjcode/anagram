CC ?= gcc
CFLAGS ?= -O3 -fPIC -Wall -Werror
LDFLAGS ?=

# Enable threading
PTHREAD_CFLAGS = -DENABLE_PTHREAD -pthread
PTHREAD_LDFLAGS = -pthread

DEFAULT = all
all: anagram spellable qwantzle wordlist

anagram: anagram.o letter_pool.o phrase_list.o sentence.o
	$(CC) -pipe -o $@ $+ $(LDFLAGS) $(PTHREAD_LDFLAGS)

anagwin: anagwin/anagwin.o anagwin/run.o anagwin/window.o \
letter_pool.o phrase_list.o sentence.o
	$(CC) -pipe -mwindows -o $@ $+ $(LDFLAGS) -lcomctl32

is_spellable: is_spellable.o letter_pool.o
	$(CC) -pipe -o $@ $+ $(LDFLAGS)

spellable: spellable.o letter_pool.o phrase_list.o
	$(CC) -pipe -o $@ $+ $(LDFLAGS)

qwantzle: qwantzle.o letter_pool.o phrase_list.o sentence.o
	$(CC) -pipe -o $@ $+ $(LDFLAGS) $(PTHREAD_LDFLAGS)

wordlist: wordlist.o letter_pool.o phrase_list.o
	$(CC) -pipe -o $@ $+ $(LDFLAGS)

# Special rules for source files using pthreads
anagram.o: anagram.c
	$(CC) -pipe -c $(CFLAGS) $(PTHREAD_CFLAGS) -o $@ $<
qwantzle.o: qwantzle.c
	$(CC) -pipe -c $(CFLAGS) $(PTHREAD_CFLAGS) -o $@ $<

%.o: %.c
	$(CC) -pipe -c $(CFLAGS) -o $@ $<
anagwin/%.o: anagwin/%.c
	$(CC) -pipe -c $(CFLAGS) -I. -o $@ $<

clean:
	rm -f anagram is_spellable spellable qwantzle wordlist *.exe *.o
	rm -f anagwin/*.o
