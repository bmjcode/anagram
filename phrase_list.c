/*
 * Functions for working with an in-memory phrase list.
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "phrase_list.h"

struct phrase_list *
phrase_list_add(struct phrase_list *prev, const char *phrase, size_t *count)
{
    struct phrase_list *next;
    const char *c;
    size_t i, length, letter_count, digit_count;

    if (phrase == NULL)
        return NULL;

    /* We need the total length for memory allocation, and the letter and
     * digit counts to determine if this is a usable phrase */
    length = 0;
    letter_count = 0;
    digit_count = 0;

    /* fgets(), which we use in phrase_list_read(), includes the trailing
     * newline character. We don't want this, so we treat it as a delimiter,
     * thus excluding it from our character count. This lets us collect all
     * our statistics while removing newlines in a single pass. */
    for (c = phrase; !((*c == '\0') || (*c == '\n')); ++c) {
        ++length;
        if (pool_in_alphabet(*c))
            ++letter_count;
        else if ((*c >= '0') && (*c <= '9'))
            ++digit_count;
    }

    /* Skip phrases with too few letters or too many digits */
    if ((letter_count == 0) || (digit_count > 0))
        return NULL;

    next = malloc(sizeof(struct phrase_list));
    if (next == NULL)
        goto failsafe;
    next->phrase = malloc((length + 1) * sizeof(char));
    if (next->phrase == NULL)
        goto failsafe;
    next->length = length;
    next->next = NULL;

    /* We can't rely on a safe function like strlcpy() being available,
     * so we'll just copy the string ourselves. In this case we don't have
     * to overthink it because we already know where the source string
     * terminates -- otherwise, we'd still be stuck on strlen(). */
    for (i = 0; i < length; ++i)
        next->phrase[i] = phrase[i];
    next->phrase[length] = '\0';

    if (prev != NULL)
        prev->next = next;

    if (count != NULL)
        ++*count;

    return next;

failsafe:
    if (next != NULL) {
        if (next->phrase != NULL)
            free(next->phrase);
        free(next);
    }
    return NULL;
}

void
phrase_list_free(struct phrase_list *first)
{
    struct phrase_list *curr, *next;

    curr = first;
    while (curr != NULL) {
        if (curr->phrase != NULL)
            free(curr->phrase);

        next = curr->next;
        free(curr);
        curr = next;
    }
}

struct phrase_list *
phrase_list_read(struct phrase_list *prev,
                 FILE *fp,
                 size_t *count,
                 pool_t *letter_pool)
{
    struct phrase_list *head, *curr;
    char buf[64]; /* that should be long enough for most words */

    if (fp == NULL)
        return NULL;

    head = NULL;
    while (fgets(buf, sizeof(buf), fp) != NULL) {
        if (!((letter_pool == NULL)
              || pool_can_spell(letter_pool, buf)))
            continue;

        curr = phrase_list_add(prev, buf, count);
        if (curr == NULL)
            continue;
        else
            prev = curr;

        /* Save a pointer to the first phrase */
        if (head == NULL)
            head = curr;
    }

    return head;
}

const char *
phrase_list_default(void)
{
    /* FIXME: There has to be a safer way to locate this file */
    if (access("web2.txt", R_OK) == 0)
        return "web2.txt";
#ifdef __unix__
    else if (access("/usr/share/dict/words", R_OK) == 0)
        return "/usr/share/dict/words";
#endif
    else
        return NULL;
}
