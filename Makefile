CC ?= gcc
CFLAGS ?= -O3 -fPIC -Wall -Werror
LDFLAGS ?=

DEFAULT = word_search
all: word_search

letter_pool: letter_pool.c
	$(CC) -DTEST_LETTER_POOL -o $@ $+ $(LDFLAGS)

word_search: word_search.o letter_pool.o util.o
	$(CC) -o $@ $+ $(LDFLAGS)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f letter_pool word_search *.exe *.o
