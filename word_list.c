/*
 * Functions for working with an in-memory word list.
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

#include "word_list.h"

struct word_list *
word_list_add(struct word_list *prev, const char *word)
{
    struct word_list *next;
    const char *c;

    next = malloc(sizeof(struct word_list));
    if (next == NULL)
        return NULL; /* failed to allocate memory */

    next->length = 0;
    next->next = NULL;

    /* Figure out how much memory to allocate */
    for (c = word; !((*c == '\0') || (*c == '\n')); ++c)
        ++next->length;

    /* Allocate memory */
    next->word = malloc((next->length + 1) * sizeof(char));
    if (next->word == NULL) {
        free(next);
        return NULL;
    }

    /* Copy the word into our own memory */
    if (strncpy(next->word, word, next->length) == NULL) {
        free(next->word);
        free(next);
        return NULL;
    }
    next->word[next->length] = '\0';

    /* If there's a previous item, link it to this one */
    if (prev != NULL)
        prev->next = next;

    return next;
}

void
word_list_free(struct word_list *first)
{
    struct word_list *curr, *next;

    curr = first;
    while (curr != NULL) {
        if (curr->word != NULL)
            free(curr->word);

        next = curr->next;
        free(curr);
        curr = next;
    }
}

struct word_list *
word_list_read(struct word_list *prev, FILE *fp)
{
    struct word_list *head, *curr;
    char buf[64]; /* that should be long enough for most words */

    head = NULL;
    while (fgets(buf, sizeof(buf), fp) != NULL) {
        /* Add the word to the list */
        curr = word_list_add(prev, buf);
        if (curr == NULL) {
            if (head != NULL)
                word_list_free(head);
            return NULL;
        } else
            prev = curr;

        /* Save a pointer to the first word */
        if (head == NULL)
            head = curr;
    }

    return head;
}
