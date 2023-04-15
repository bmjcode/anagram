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

#include "sentence.h"

void
sentence_info_init(struct sentence_info *si)
{
    if (si == NULL)
        return;

    si->phrase_list = NULL;
    si->phrase_count = 0;
    si->pool = NULL;
    si->sentence = NULL;
    si->length = 0;
    si->depth = 0;

    si->check_cb = NULL;
    si->done_cb = NULL;
    si->user_data = NULL;
}

void
sentence_build(struct sentence_info *si)
{
    struct phrase_list *curr;

    if ((si == NULL) || (si->pool == NULL))
        return;
    else if (!((si->check_cb == NULL) || si->check_cb(si)))
        return;

    if (pool_is_empty(si->pool) && (si->sentence != NULL)) {
        /* We've completed a sentence */
        if (si->done_cb == NULL)
            printf("%s\n", si->sentence);
        else
            si->done_cb(si);
        return;
    } else if (si->phrase_list == NULL)
        return;

    for (curr = si->phrase_list; curr != NULL; curr = curr->next) {
        struct sentence_info nsi;
        char *n, *p;

        if (curr->phrase == NULL)
            break; /* best to assume something's really gone wrong here */

        if (!pool_can_spell(si->pool, curr->phrase))
            continue;

        /* Remove this phrase's letters from the pool */
        pool_subtract(si->pool, curr->phrase);

        sentence_info_init(&nsi);
        nsi.pool = si->pool;
        nsi.depth = si->depth;
        nsi.check_cb = si->check_cb;
        nsi.done_cb = si->done_cb;
        nsi.user_data = si->user_data;

        /* Remove words we can no longer spell from the phrase list.
         * Note a NULL value here can mean success (we've used all the
         * letters in the pool) or failure (the memory allocation failed).
         * We will determine which it is on the next recursive call. */
        nsi.phrase_list = phrase_list_filter(si->phrase_list,
                                             &nsi.phrase_count,
                                             nsi.pool);

        if (si->sentence == NULL)
            nsi.length = curr->length;
        else
            nsi.length = si->length + curr->length + 1; /* add a space */

        nsi.sentence = malloc((nsi.length + 1) * sizeof(char));
        if (nsi.sentence == NULL)
            break;

        /* Inlining this avoids strcat()'s excessive seeking */
        /* This isn't overflow-safe but neither is anything else here */
        n = nsi.sentence;
        if (si->sentence != NULL) {
            p = si->sentence;
            while (*p != '\0')
                *n++ = *p++;
            *n++ = ' ';
        }
        p = curr->phrase;
        while (*p != '\0')
            *n++ = *p++;
        *n = '\0';

        /* Extend the sentence with our new phrase */
        sentence_build(&nsi);
        free(nsi.sentence);
        phrase_list_filter_free(nsi.phrase_list);

        /* Restore the pool for the next cycle */
        pool_add(si->pool, curr->phrase);
    }
}
