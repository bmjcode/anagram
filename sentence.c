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
}

void
sentence_build(struct sentence_info *si)
{
    struct phrase_list *curr;

    if ((si == NULL) || (si->phrase_list == NULL) || (si->pool == NULL))
        return;

    if (pool_is_empty(si->pool) && (si->sentence != NULL)) {
        /* We've completed a sentence */
        printf("%s\n", si->sentence);
        return;
    }

    for (curr = si->phrase_list; curr != NULL; curr = curr->next) {
        struct sentence_info nsi;
        char *n, *p;

        if (curr->phrase == NULL)
            break; /* best to assume something's really gone wrong here */

        if (!pool_can_spell(si->pool, curr->phrase))
            continue;

        sentence_info_init(&nsi);
        nsi.phrase_list = si->phrase_list;
        nsi.pool = si->pool;

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

        /* Remove this phrase's letters from the pool */
        pool_subtract(si->pool, curr->phrase);

        /* Extend the sentence with our new phrase */
        sentence_build(&nsi);
        free(nsi.sentence);

        /* Restore the pool for the next cycle */
        pool_add(si->pool, curr->phrase);
    }
}
