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
#include <string.h>

#include "phrase_list.h"

struct phrase_list *
phrase_list_add(struct phrase_list *prev, const char *phrase, size_t *count)
{
    struct phrase_list *next;
    const char *c;

    next = malloc(sizeof(struct phrase_list));
    if (next == NULL)
        return NULL;

    next->length = 0;
    next->next = NULL;

    /* Copy the phrase into our own memory */
    for (c = phrase; !((*c == '\0') || (*c == '\n')); ++c)
        ++next->length;

    next->phrase = malloc((next->length + 1) * sizeof(char));
    if (next->phrase == NULL) {
        free(next);
        return NULL;
    }

    if (strncpy(next->phrase, phrase, next->length) == NULL) {
        free(next->phrase);
        free(next);
        return NULL;
    }
    next->phrase[next->length] = '\0';

    if (prev != NULL)
        prev->next = next;

    if (count != NULL)
        ++*count;

    return next;
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
phrase_list_read(struct phrase_list *prev, FILE *fp, size_t *count)
{
    struct phrase_list *head, *curr;
    char buf[64]; /* that should be long enough for most words */

    head = NULL;
    while (fgets(buf, sizeof(buf), fp) != NULL) {
        curr = phrase_list_add(prev, buf, count);
        if (curr == NULL) {
            if (head != NULL)
                phrase_list_free(head);
            return NULL;
        } else
            prev = curr;

        /* Save a pointer to the first phrase */
        if (head == NULL)
            head = curr;
    }

    return head;
}
