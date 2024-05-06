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

#include "sentence.h"

/* Internal state for sentence_build_inner() */
struct sbi_state {
    char *sentence;
    char *write_pos;
    char **phrases;
    size_t phrase_count;
    size_t depth; /* recursion depth; also the number of words we've used */
};

static void sentence_build_inner(struct sentence_info *si,
                                 struct sbi_state *sbi);

void
sentence_info_init(struct sentence_info *si)
{
    if (si == NULL)
        return;

    pool_reset(si->pool);

    si->phrase_list = NULL;
    si->phrase_count = 0;
    si->user_data = NULL;

    si->max_words = 0;

    si->step = 1;
    si->offset = 0;

    si->canceled_cb = NULL;
    si->phrase_filter_cb = NULL;
    si->first_phrase_cb = NULL;
    si->progress_cb = NULL;
    si->sentence_cb = NULL;
    si->finished_cb = NULL;
}

void
sentence_build(struct sentence_info *si)
{
    struct sbi_state sbi;
    struct phrase_list *lp; /* list pointer */
    char **dst;

    if ((si == NULL)
        || pool_is_empty(si->pool)
        || (si->phrase_list == NULL)
        || (si->phrase_count == 0))
        return;

    sbi.depth = 0;

    /* Allocate enough memory for the longest possible sentence:
     * all single-letter words with a space or '\0' after each. */
    sbi.sentence = malloc(2 * pool_count_all(si->pool) * sizeof(char));
    if (sbi.sentence == NULL)
        return; /* this could be a problem */

    /* This is the position where we add the next word in the sentence */
    sbi.write_pos = sbi.sentence;
    *sbi.write_pos = '\0';

    sbi.phrase_count = si->phrase_count;
    sbi.phrases = malloc((sbi.phrase_count + 1) * sizeof(char*));
    if (sbi.phrases == NULL)
        return; /* this may be a problem */

    /* Flatten the phrase list into an array of char* pointers with a
     * terminating NULL pointer. We read it in as a linked list because
     * we don't know the number of items in advance, and iterating
     * through again to count would be either slow (reading from disk)
     * or impossible (reading from stdin). Once we reach our inner loop
     * our slowest operation is memory allocation, and duplicating an
     * array -- which we do for each iteration -- only requires one of
     * those as opposed to many for a linked list. */
    for (lp = si->phrase_list, dst = sbi.phrases;
         lp != NULL;
         lp = lp->next) {
        /* Filter the list now since we're iterating through it anyway */
        if ((si->phrase_filter_cb == NULL)
            || si->phrase_filter_cb(lp->phrase, si->user_data))
            *dst++ = lp->phrase;
        else
            --sbi.phrase_count;
    }
    *dst = NULL;

    sentence_build_inner(si, &sbi);
    if (si->finished_cb != NULL)
        si->finished_cb(si->user_data);

    free(sbi.sentence);
    free(sbi.phrases);
}

void sentence_build_inner(struct sentence_info *si,
                          struct sbi_state *sbi)
{
    char **prev, **curr, *n, *p;
    size_t i;

    /*
     * We can skip the sanity checks here because this function is only
     * ever called from sentence_build(), which already did them, or
     * recursively from itself. If something was going to overflow, it
     * would have done so long before we got here.
     */

    if ((si->canceled_cb != NULL) && si->canceled_cb(si->user_data))
        return;

    /* Filter our working list to remove phrases we can't spell with the
     * letters in the current pool. */
    for (prev = curr = sbi->phrases; *prev != NULL; ++prev) {
        if (pool_can_spell(si->pool, *prev))
            *curr++ = *prev;
        else
            --sbi->phrase_count;
    }
    *curr = NULL;

    curr = sbi->phrases;
    if (sbi->depth == 0) {
        /* If this is the outermost loop, skip the number of phrases
         * specified by 'offset'. */
        for (i = 0; i < si->offset; ++i) {
            ++curr;
            if (*curr == NULL)
                return;
        }
    }

    while (*curr != NULL) {
        if ((si->canceled_cb != NULL) && si->canceled_cb(si->user_data))
            break;

        /* If this is the outermost loop, report our new first phrase. */
        if ((sbi->depth == 0) && (si->first_phrase_cb != NULL))
            si->first_phrase_cb(*curr, si->user_data);

        /* Remove this phrase's letters from the pool. */
        pool_subtract(si->pool, *curr);

        /* Add this phrase to our sentence. */
        n = sbi->write_pos;
        p = *curr;
        while (*p != '\0')
            *n++ = *p++;

        if (pool_is_empty(si->pool)) {
            /* We've completed a sentence! */
            *n = '\0';
            if (si->sentence_cb == NULL)
                printf("%s\n", sbi->sentence);
            else
                si->sentence_cb(sbi->sentence, si->user_data);
        } else if ((si->max_words == 0)
                   || (sbi->depth + 1 < si->max_words)) {
            struct sbi_state new_sbi;
            size_t buf_size = (sbi->phrase_count + 1) * sizeof(char*);

            new_sbi.sentence = sbi->sentence;
            new_sbi.phrases = malloc(buf_size);
            if (new_sbi.phrases != NULL) {
                memcpy(new_sbi.phrases, sbi->phrases, buf_size);
                new_sbi.phrase_count = sbi->phrase_count;
                new_sbi.depth = sbi->depth + 1;

                *n++ = ' ';
                *n = '\0'; /* hide remnants of previous attempts */
                new_sbi.write_pos = n;

                /* Call this function recursively to extend the sentence. */
                sentence_build_inner(si, &new_sbi);
                free(new_sbi.phrases);
            }
        }

        /* Restore used letters to the pool for the next cycle. */
        pool_add(si->pool, *curr);

        /* If this is the outermost loop, report our progress. */
        if ((sbi->depth == 0) && (si->progress_cb != NULL))
            si->progress_cb(si->user_data);

        /* If this is the outermost loop, advance by the number of phrases
         * specified by 'step'. Otherwise, advance to the next phrase. */
        for (i = 0; i < (sbi->depth == 0 ? si->step : 1); ++i) {
            ++curr;
            if (*curr == NULL)
                break;
        }
    }
}
