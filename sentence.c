/*
 * Functions for building sentences from phrases.
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
    size_t length;
    char *write_pos;
    struct phrase_list **phrases;
    size_t phrase_count;
    size_t depth; /* recursion depth */
    size_t used_words; /* individual words, not complete phrases! */
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
    si->add_phrase_cb = NULL;
    si->first_phrase_cb = NULL;
    si->progress_cb = NULL;
    si->sentence_cb = NULL;
}

void
sentence_build(struct sentence_info *si)
{
    struct sbi_state sbi;
    struct phrase_list *lp; /* list pointer */
    struct phrase_list **dst;
    size_t i, buf_length;

    if ((si == NULL)
        || pool_is_empty(si->pool)
        || (si->phrase_list == NULL)
        || (si->phrase_count == 0))
        return;

    sbi.sentence = NULL;
    sbi.length = 0;
    sbi.phrases = NULL;
    sbi.depth = 0;
    sbi.used_words = 0;

    /* Our buffer must be big enough to hold any sentence we can build.
     * Start by assuming the worst-case scenario: all single-letter words,
     * with a space or the terminating '\0' after each. */
    buf_length = 2 * pool_count_all(si->pool);

    /* Now hold that thought while we prepare our phrase list. */
    sbi.phrase_count = si->phrase_count;
    sbi.phrases = malloc((sbi.phrase_count + 1)
                         * sizeof(struct phrase_list*));
    if (sbi.phrases == NULL) /* this could be a problem */
        goto cleanup;

    /* From here on we will iterate through our phrase list using an array
     * rather than by following its items' 'next' pointers. In our inner
     * loop, this lets us efficiently remove phrases we can no longer use
     * by replacing the array with a copy that omits them. */
    for (lp = si->phrase_list, dst = sbi.phrases;
         lp != NULL;
         lp = lp->next) {
        *dst++ = lp;
        /* Did I say our worst case was all single-letter words?
         * I forgot to mention that phrases can include non-alphabetic
         * characters like spaces and punctuation. To play it safe,
         * let's leave enough space to fit all of them at once. */
        for (i = 0; i < lp->length; ++i)
            if (!pool_in_alphabet(lp->phrase[i]))
                ++buf_length;
    }
    *dst = NULL;

    /* Now let's allocate that buffer. Reusing the same oversized buffer
     * is far more efficient than allocating a new, perfectly-sized buffer
     * each time we add a phrase. The magic of C strings is we can put a
     * '\0' wherever we want to end them and no one will be the wiser. */
    sbi.sentence = malloc(buf_length * sizeof(char));
    if (sbi.sentence == NULL) /* this could also be a problem */
        goto cleanup;

    /* This is the position where we add the next word in the sentence */
    sbi.write_pos = sbi.sentence;
    *sbi.write_pos = '\0';

    /* Now the fun begins */
    sentence_build_inner(si, &sbi);

cleanup:
    if (sbi.sentence != NULL)
        free(sbi.sentence);
    if (sbi.phrases != NULL)
        free(sbi.phrases);
}

void sentence_build_inner(struct sentence_info *si,
                          struct sbi_state *sbi)
{
    struct phrase_list **prev, **curr;
    char *n, *p;
    size_t i, wc;

    /* We can skip the sanity checks here because this function is only
     * ever called from sentence_build(), which already did them, or
     * recursively from itself. If something was going to overflow, it
     * would have done so long before we got here. */

    if ((si->canceled_cb != NULL) && si->canceled_cb(si->user_data))
        return;

    /* Filter our working list to remove phrases we can't spell with the
     * letters in the current pool. */
    for (prev = curr = sbi->phrases; *prev != NULL; ++prev) {
        if (pool_can_spell(si->pool, (*prev)->phrase))
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

        wc = 0;
        if (si->max_words != 0) {
            size_t lc;
            /* Count how many words are in this phrase, and skip it if it
             * would put us over our limit. */
            for (p = (*curr)->phrase, lc = 0;
                 /* intentionally left blank */;
                 ++p) {
                if (phrase_delimiter(*p)) {
                    if (lc >= 1) {
                        ++wc;
                        if (sbi->used_words + wc > si->max_words)
                            goto next_phrase;
                    }
                    /* We check for the end of the phrase here rather than
                     * in the for loop so our last phrase is counted above. */
                    if (phrase_terminator(*p))
                        break; /* we're done with this phrase */
                    else
                        lc = 0; /* reset the count for the next word */
                } else if (pool_in_alphabet(*p))
                    ++lc;
            }
        }

        /* Check if we can add this phrase here. */
        if (!((si->add_phrase_cb == NULL)
              || si->add_phrase_cb((*curr)->phrase, (*curr)->length,
                                   sbi->sentence, sbi->length,
                                   si->pool, si->user_data)))
            goto next_phrase;

        /* If this is the outermost loop, report our new first phrase. */
        if ((sbi->depth == 0) && (si->first_phrase_cb != NULL))
            si->first_phrase_cb((*curr)->phrase, (*curr)->length,
                                si->user_data);

        /* Remove this phrase's letters from the pool. */
        pool_subtract(si->pool, (*curr)->phrase);

        /* Add this phrase to our sentence. */
        n = sbi->write_pos;
        p = (*curr)->phrase;
        while (*p != '\0')
            *n++ = *p++;
        sbi->length = n - sbi->sentence;

        if (pool_is_empty(si->pool)) {
            /* We've completed a sentence! */
            *n = '\0';
            if (si->sentence_cb == NULL)
                printf("%s\n", sbi->sentence);
            else
                si->sentence_cb(sbi->sentence, sbi->length, si->user_data);
        } else {
            struct sbi_state new_sbi;
            size_t buf_size = (sbi->phrase_count + 1)
                              * sizeof(struct phrase_list*);

            new_sbi.sentence = sbi->sentence;
            new_sbi.phrases = malloc(buf_size);
            if (new_sbi.phrases != NULL) {
                memcpy(new_sbi.phrases, sbi->phrases, buf_size);
                new_sbi.phrase_count = sbi->phrase_count;
                new_sbi.depth = sbi->depth + 1;
                new_sbi.used_words = sbi->used_words + wc;

                *n++ = ' ';
                *n = '\0'; /* hide remnants of previous attempts */
                new_sbi.length = sbi->length + 1; /* include the space */
                new_sbi.write_pos = n;

                /* Call this function recursively to extend the sentence. */
                sentence_build_inner(si, &new_sbi);
                free(new_sbi.phrases);
            }
        }

        /* Restore used letters to the pool for the next cycle. */
        pool_add(si->pool, (*curr)->phrase);

next_phrase:
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
