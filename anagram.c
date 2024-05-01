/*
 * Find anagrams of a word or phrase.
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

static void start(struct sentence_info *si, unsigned short num_threads);
static void usage(FILE *stream, char *prog_name);

#ifdef ENABLE_PTHREAD
/* Wrapper to call sentence_build() in a thread */
static void *run_thread(void *si);
#endif /* ENABLE_PTHREAD */

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
            "Find anagrams of a word or phrase.\n"
            "Usage: %s [-h] [-f] [-l PATH] [-t NUM] [-w NUM] subject\n"
            "  -h       Display this help message and exit\n"
            "  -f       Filter mode (read phrase list from stdin)\n"
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
    int i, opt;
    unsigned short num_threads;

    sentence_info_init(&si);

    fp = NULL;
    list_path = NULL;
    num_threads = 1;

    while ((opt = getopt(argc, argv, "hfl:t:w:")) != -1) {
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

    /* The remaining command-line arguments specify the subject */
    if (optind >= argc) {
        usage(stderr, argv[0]);
        return 1;
    }
    for (i = optind; i < argc; ++i)
        pool_add(si.pool, argv[i]);

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
            return 1;
        }
    }
    si.phrase_list = phrase_list_read(NULL, fp, &si.phrase_count, si.pool);
    fclose(fp);

    if (si.phrase_list == NULL) {
        fprintf(stderr,
                "Failed to read phrase list: %s\n",
                list_path);
        return 1;
    }

    /* Search for valid sentences */
    start(&si, num_threads);

    phrase_list_free(si.phrase_list);
    return 0;
}
