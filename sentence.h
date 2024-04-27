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

#ifndef SENTENCE_H
#define SENTENCE_H

#include <stdbool.h>

#include "letter_pool.h"
#include "phrase_list.h"

struct sentence_info;

/*
 * Callback function to perform a validation check on a sentence.
 * Return true to accept, false to reject.
 * If none is specified, all potential sentences are accepted.
 */
typedef bool (*sentence_check_cb)(struct sentence_info *si,
                                  char *newest_phrase);

/*
 * Callback function when a sentence is completed.
 * If none is specified, the completed sentence is printed to stdout.
 */
typedef void (*sentence_done_cb)(struct sentence_info *si);

/*
 * Structure representing the state of sentence_build().
 *
 * To access the completed sentence, assign a callback function to done_cb.
 * When this function is called, the sentence will be in its si->sentence.
 * Beware that this is the ONLY way to access the sentence, as the 'sentence'
 * member otherwise contains working state that is not necessarily safe for
 * outside use. Its memory is freed when sentence_build() returns, so copy it
 * to your own memory if you need it longer.
 */
struct sentence_info {
    struct phrase_list *phrase_list;
    size_t phrase_count;
    pool_t *pool;
    char *sentence;
    size_t max_length; /* used as the buffer size for 'sentence' */

    /* Use these to impose constraints on sentence building */
    size_t max_words; /* max number of words in a sentence (0 for unlimited) */

    /* Callback functions */
    sentence_check_cb check_cb;
    sentence_done_cb done_cb;
    void *user_data; /* user-specified arbitrary data */

    /* Use these to divide the phrase list among multiple threads */
    size_t step;    /* use every nth word */
    size_t offset;  /* skip the first n words */
};

/*
 * Initialize a sentence_info structure.
 */
void sentence_info_init(struct sentence_info *si, pool_t *pool);

/*
 * Build a "sentence" using phrases formed from letters in the pool.
 * For our purposes a sentence is any combination of one or more phrases
 * separated by spaces.
 */
void sentence_build(struct sentence_info *si);

/*
 * Divide sentence_build() across multiple threads.
 *
 * If your system does not support pthreads, this always starts a
 * single thread.
 */
void sentence_build_threaded(struct sentence_info *si,
                             unsigned short num_threads);

#endif /* SENTENCE_H */
