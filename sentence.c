/*
 * Functions for building a sentence from a phrase list.
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

#ifdef ENABLE_THREADING
#  include <pthread.h>
#  include <signal.h>
#endif /* ENABLE_THREADING */

#include "sentence.h"

static void sentence_build_inner(struct sentence_info *si,
                                 char *write_pos,
                                 struct phrase_list *phrase_list,
                                 size_t phrase_count,
                                 size_t depth);
#ifdef ENABLE_THREADING
/* Wrapper to call sentence_build() in a thread */
static void *run_thread(void *si);
#endif /* ENABLE_THREADING */

void
sentence_info_init(struct sentence_info *si, pool_t *pool)
{
    if (si == NULL)
        return;

    si->phrase_list = NULL;
    si->phrase_count = 0;
    si->pool = pool;
    si->sentence = NULL;
    si->max_length = 0;

    si->check_cb = NULL;
    si->done_cb = NULL;
    si->user_data = NULL;

    si->step = 1;
    si->offset = 0;
    si->limit = 0;
}

void
sentence_build(struct sentence_info *si)
{
    if ((si == NULL) || (si->pool == NULL))
        return;

    /* Allocate enough memory for the longest possible sentence:
     * all single-letter words with a space or '\0' after each. */
    si->max_length = 2 * pool_count(si->pool);
    si->sentence = malloc(si->max_length * sizeof(char));
    if (si->sentence == NULL)
        return; /* this could be a problem */
    memset(si->sentence, 0, si->max_length * sizeof(char));

    sentence_build_inner(si, NULL, si->phrase_list, si->phrase_count, 0);
    free(si->sentence);
}

void sentence_build_inner(struct sentence_info *si,
                          char *write_pos,
                          struct phrase_list *phrase_list,
                          size_t phrase_count,
                          size_t depth)
{
    struct phrase_list *curr, *new_list;
    size_t i, phrases_used, new_count;
    char *n, *p;

    if ((si == NULL) || (si->pool == NULL) || (phrase_list == NULL))
        return;

    /* We'll come back to this */
    new_list = NULL;
    new_count = 0;

    /* Skip the number of phrases specified by 'offset'. */
    curr = phrase_list;
    for (i = 0; i < si->offset; ++i) {
        curr = curr->next;
        if (curr == NULL)
            return;
    }

    /* Add the next word starting at this position in si->sentence. */
    if (write_pos == NULL)
        write_pos = si->sentence;

    phrases_used = 0;
    while (curr != NULL) {
        if (curr->phrase == NULL)
            break; /* best to assume something's really gone wrong here */

        /* Skip phrases that we can't spell with our letter pool
         * or that don't pass our validation check. */
        if (!pool_can_spell(si->pool, curr->phrase))
            continue;
        else if (!((si->check_cb == NULL)
                   || si->check_cb(si, curr)))
            continue;

        /* Remove this phrase's letters from the pool. */
        pool_subtract(si->pool, curr->phrase);

        /* Add this phrase to our sentence.
         * Copying bytes manually is more efficient than using the standard
         * library functions, whose execution time grows proportionally with
         * the length of the destination string. We can safely assume this
         * won't overflow because something else would have already overflowed
         * long before we got here. */
        n = write_pos;
        p = curr->phrase;
        while (*p != '\0')
            *n++ = *p++;

        if (pool_is_empty(si->pool)) {
            *n = '\0';
            /* We've completed a sentence! */
            if (si->done_cb == NULL)
                printf("%s\n", si->sentence);
            else
                si->done_cb(si);
        } else {
            *n++ = ' ';

            /* Remove phrases we can no longer spell from the list.
             * Note a NULL value here can mean success (we've used all the
             * letters in the pool) or failure (the memory allocation failed).
             * We will determine which it is on the next recursive call. */
            new_count = phrase_count;
            new_list = phrase_list_filter(phrase_list, &new_count, si->pool);

            /* Call this function recursively to extend the sentence. */
            sentence_build_inner(si, n, new_list, new_count, depth + 1);
            phrase_list_filter_free(new_list);
        }

        /* Restore used letters to the pool for the next cycle. */
        pool_add(si->pool, curr->phrase);

        /* If we have a limit on the number of phrases to process,
         * stop when we've reached it. */
        ++phrases_used;
        if ((si->limit > 0) && (phrases_used >= si->limit))
            break;

        /* Advance by the number of phrases specified by 'step'. */
        for (i = 0; i < si->step; ++i) {
            curr = curr->next;
            if (curr == NULL)
                break;
        }
    }
}

void
sentence_build_threaded(struct sentence_info *si,
                        unsigned short num_threads)
{
#ifdef ENABLE_THREADING
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
        tsi[i]->limit = tsi[i]->phrase_count / num_threads;

        /* Each thread needs its own letter pool */
        tsi[i]->pool = malloc(POOL_SIZE * sizeof(pool_t));
        if (tsi[i]->pool == NULL)
            goto cleanup;
        pool_copy(si->pool, tsi[i]->pool);
    }

    /* If the phrase list doesn't divide evenly, distribute the remainder
     * among the earliest threads */
    for (i = 0; i < si->phrase_count % num_threads; ++i)
        ++(tsi[i]->limit);

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
            if (tsi[i] != NULL) {
                if (tsi[i]->pool != NULL)
                    free(tsi[i]->pool);
                free(tsi[i]);
            }
        }
        free(tsi);
    }
#else /* ENABLE_THREADING */
    if (num_threads <= 0) {
        fprintf(stderr,
                "Invalid number of threads: %d\n",
                num_threads);
        return;
    } else if (num_threads > 1)
        fprintf(stderr,
                "Warning: Threading unavailable\n");
    sentence_build(si);
#endif /* ENABLE_THREADING */
}

#ifdef ENABLE_THREADING
void *
run_thread(void *si)
{
    sentence_build(si);
    return NULL;
}
#endif /* ENABLE_THREADING */
