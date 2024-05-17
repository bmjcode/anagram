/*
 * Solve the anacryptogram from Dinosaur Comics #1663 (aka the Qwantzle).
 * Copyright (c) 2023 Benjamin Johnson <bmjcode@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef ENABLE_PTHREAD
#  include <pthread.h>
#  include <signal.h>
#endif /* ENABLE_PTHREAD */

#include "letter_pool.h"
#include "phrase_list.h"
#include "sentence.h"

enum mode {
    QWANTZLE_SOLVER,
    PHRASE_FILTER
};

static size_t qwantzle_phrase_filter(char *candidate, pool_t *pool,
                                     void *user_data);
static bool qwantzle_add_phrase(char *candidate, char *sentence, pool_t *pool,
                                void *user_data);
static void qwantzle_solved(char *sentence, void *user_data);

static void start(struct sentence_info *si, unsigned short num_threads);
static void usage(FILE *stream, char *prog_name);

#ifdef ENABLE_PTHREAD
/* Wrapper to call sentence_build() in a thread */
static void *run_thread(void *si);
#endif /* ENABLE_PTHREAD */

#define isdelim(c) (((c) == ' ') || phrase_terminator(c))

size_t
qwantzle_phrase_filter(char *candidate, pool_t *pool, void *user_data)
{
    char *c;
    size_t length, lc;  /* letter count */
    pool_t antipool[POOL_SIZE];

    if ((candidate == NULL) || (pool == NULL))
        return 0;

    pool_reset(antipool);

    /* Count the letters in each word of this phrase */
    for (c = candidate, length = 0, lc = 0;
         /* intentionally left blank */;
         ++c) {
        if (isdelim(*c)) {
            /* The two longest words have 11 and 8 letters, respectively */
            if ((lc < 1) /* how? */
                || (lc > 11)
                || ((lc > 8) && (lc < 11)))
                return 0;

            if (phrase_terminator(*c))
                break;  /* we've reached the end of the phrase */
            else
                lc = 0; /* reset the letter count for the next word */
        } else if (pool_in_alphabet(*c)) {
            pool_add_letter(antipool, *c);
            if (pool_count(antipool, *c) > pool_count(pool, *c))
                return 0;
            else if ((*c == 'w')
                     && (pool_count(antipool, *c) == pool_count(pool, *c))
                     && (!phrase_terminator(*(c + 1))))
                /* The final letter is 'w', so eliminate phrases that could
                 * never work because they use all ours up too early */
                return 0;
            ++lc;
        } else if (phrase_cannot_include(*c))
            return 0;
        ++length;
    }

    /* "I" and "a" (case-sensitive) are the only plausible one-letter words */
    if ((length == 1)
        && !((candidate[0] == 'I') || (candidate[0] == 'a')))
        return 0;

    return length;
}

bool
qwantzle_add_phrase(char *candidate, char *sentence, pool_t *pool,
                    void *user_data)
{
    char *c, *s;
    size_t clc, slc;    /* candidate and sentence letter counts */
    pool_t antipool[POOL_SIZE];

    pool_reset(antipool);

    for (c = candidate, clc = 0;
         /* intentionally left blank */;
         ++c) {
        if (isdelim(*c)) {
            if ((clc == 8) || (clc == 11)) {
                /* It's implied there is only one word of each length */
                for (s = sentence, slc = 0;
                     /* intentionally left blank */;
                     ++s) {
                    if (isdelim(*s)) {
                        if (clc == slc)
                            return false;
                        else if (*s == '\0')
                            break;
                        else
                            slc = 0;
                    } else if (pool_in_alphabet(*s))
                        ++slc;
                }
            }

            if (*c == '\0')
                break;
            else
                clc = 0;
        } else if (pool_in_alphabet(*c)) {
            pool_add_letter(antipool, *c);
            if ((*c == 'w')
                && (pool_count(antipool, 'w') == pool_count(pool, 'w'))
                && !pool_counts_match(antipool, pool))
                /* This isn't our last phrase, so don't use up our 'w's yet */
                return false;
            ++clc;
        }
    }

    if (pool_counts_match(antipool, pool)) {
        /* This would be our last phrase, so check if it ends with 'w' */
        while (!pool_in_alphabet(*c))
            --c;
        return (*c == 'w');
    } else
        return true;
}

void
qwantzle_solved(char *sentence, void *user_data)
{
    struct phrase_list *guess = user_data;

    /* Hard-code the first word and end punctuation */
    printf("%s", "I");
    for (; guess != NULL; guess = guess->next)
        printf(" %s", guess->phrase);
    printf(" %s!!\n", sentence);
}

void
start(struct sentence_info *si, unsigned short num_threads)
{
#ifdef ENABLE_PTHREAD
    int s;
    unsigned short i;
    pthread_t *thread_id;
    pthread_attr_t attr;
    struct sentence_info **tsi;

    if (si == NULL)
        return;

    if (num_threads <= 0) {
        fprintf(stderr,
                "Invalid number of threads: %d\n",
                num_threads);
        return;
    } else if (num_threads == 1) {
        /* We don't need all this overhead for one thread */
        sentence_build(si);
        return;
    }

    thread_id = NULL;
    tsi = NULL;

    s = pthread_attr_init(&attr);
    if (s != 0)
        goto cleanup;

    thread_id = malloc(num_threads * sizeof(pthread_t));
    if (thread_id == NULL)
        goto cleanup;

    /* Create a struct sentence_info for each thread */
    tsi = malloc(num_threads * sizeof(struct sentence_info*));
    if (tsi == NULL)
        goto cleanup;

    for (i = 0; i < num_threads; ++i) {
        tsi[i] = malloc(sizeof(struct sentence_info));
        if (tsi[i] == NULL)
            goto cleanup;

        /* Divide the phrase list among the threads */
        memcpy(tsi[i], si, sizeof(struct sentence_info));
        tsi[i]->step = num_threads;
        tsi[i]->offset = i;
    }

    /* Start each thread */
    for (i = 0; i < num_threads; ++i) {
        s = pthread_create(&thread_id[i],
                           &attr,
                           run_thread,
                           tsi[i]);
        if (s != 0) {
            while (--i >= 0)
                pthread_kill(thread_id[i], SIGTERM);
            goto cleanup;
        }
    }

    /* Wait for each thread to finish */
    for (i = 0; i < num_threads; ++i) {
        s = pthread_join(thread_id[i], NULL);
        if (s != 0) {
            while (--i >= 0)
                pthread_kill(thread_id[i], SIGTERM);
            goto cleanup;
        }
    }

cleanup:
    pthread_attr_destroy(&attr);

    if (thread_id != NULL)
        free(thread_id);

    if (tsi != NULL) {
        for (i = 0; i < num_threads; ++i) {
            if (tsi[i] != NULL)
                free(tsi[i]);
        }
        free(tsi);
    }
#else /* ENABLE_PTHREAD */
    if (num_threads <= 0) {
        fprintf(stderr,
                "Invalid number of threads: %d\n",
                num_threads);
        return;
    } else if (num_threads > 1)
        fprintf(stderr,
                "Warning: Threading unavailable\n");
    sentence_build(si);
#endif /* ENABLE_PTHREAD */
}

void
usage(FILE *stream, char *prog_name)
{
    fprintf(stream,
            "Solve the anacryptogram from Dinosaur Comics #1663.\n"
            "Usage: %s [-h] [-f] [-p] [-l PATH] [-t NUM] [-w NUM] "
            "[guess ...]\n"
            "  -h       Display this help message and exit\n"
            "  -f       Filter mode (read phrase list from stdin)\n"
            "  -p       Print valid phrases from the list and exit\n"
            "  -l PATH  Override the default phrase list\n"
            "  -t NUM   Start the specified number of threads (default: 1)\n"
            "  -w NUM   Limit results to this many words or fewer\n",
            prog_name);
}

#ifdef ENABLE_PTHREAD
void *
run_thread(void *si)
{
    sentence_build(si);
    return NULL;
}
#endif /* ENABLE_PTHREAD */

int
main(int argc, char **argv)
{
    FILE *fp;
    struct sentence_info si;
    const char *list_path;
    int i, opt, retval;
    unsigned short mode, num_threads;
    struct phrase_list *guessed_words, *last_guess;

    retval = 0;
    mode = QWANTZLE_SOLVER;
    guessed_words = NULL;
    last_guess = NULL;

    sentence_info_init(&si);
    si.add_phrase_cb = qwantzle_add_phrase;
    si.sentence_cb = qwantzle_solved;

    /* The first word of the solution, "I", is added by qwantzle_solved(),
     * so we only have to solve for the remaining letters. */
    pool_add(si.pool,
             "ttttttttttttooooooooooeeeeeeeeaaaaaaallllllnnnnnn"
             "uuuuuuiiiiisssssdddddhhhhhyyyyyIIrrrfffbbwwkcmvg");

    fp = NULL;
    list_path = NULL;
    num_threads = 1;

    while ((opt = getopt(argc, argv, "hfpl:t:w:")) != -1) {
        switch (opt) {
            case 'h':
                /* Display help and exit */
                (void)num_threads;
                usage(stdout, argv[0]);
                return 0;
            case 'f':
                /* Filter mode */
                fp = stdin;
                break;
            case 'p':
                /* Print filtered phrases to stdout */
                mode = PHRASE_FILTER;
                break;
            case 'l':
                /* Override the default phrase list */
                list_path = optarg;
                break;
            case 't':
                /* Set the number of threads */
                num_threads = strtoul(optarg, NULL, 0);
                break;
            case 'w':
                /* Set the maximum number of words */
                si.max_words = strtoul(optarg, NULL, 0);
                break;
        }
    }

    /* Treat the remaining command-line arguments as guesses */
    for (i = optind; i < argc; ++i) {
        char *phrase = argv[i];
        size_t length = qwantzle_phrase_filter(phrase, si.pool, NULL);
        if ((i == optind) && !strcmp(phrase, "I")) {
            /* We don't have to guess this because we already know it */
            continue;
        } else if (length == 0) {
            fprintf(stderr, "Ignoring invalid guess: \"%s\"\n", phrase);
            continue;
        }
        last_guess = phrase_list_add(last_guess, phrase, length, NULL);
        if (last_guess == NULL) {
            /* If we're out of memory already, we're in trouble */
            goto cleanup;
        } else if (guessed_words == NULL) {
            guessed_words = last_guess;
            si.user_data = guessed_words;
        }
        pool_subtract(si.pool, phrase);
    }

    if (list_path == NULL)
        list_path = phrase_list_default();
    else if (strcmp(list_path, "-") == 0)
        fp = stdin;

    if (fp == NULL) {
        fp = fopen(list_path, "r");
        if (fp == NULL) {
            fprintf(stderr,
                    "Failed to open: %s\n",
                    list_path);
            retval = 1;
            goto cleanup;
        }
    }
    si.phrase_list = phrase_list_read_filtered(
        NULL, fp, &si.phrase_count, si.pool, qwantzle_phrase_filter, NULL);
    fclose(fp);

    if (si.phrase_list == NULL) {
        fprintf(stderr,
                "Failed to read phrase list: %s\n",
                list_path);
        retval = 1;
        goto cleanup;
    }

    if (mode == PHRASE_FILTER)
        phrase_list_write(si.phrase_list, stdout);
    else
        start(&si, num_threads);

cleanup:
    if (si.phrase_list != NULL)
        phrase_list_free(si.phrase_list);
    if (guessed_words != NULL)
        phrase_list_free(guessed_words);
    return retval;
}
