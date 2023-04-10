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
word_list_add(struct word_list *prev, char *word)
{
    struct word_list *next;

    next = malloc(sizeof(struct word_list));
    if (next == NULL)
        return NULL; /* failed to allocate memory */

    next->word = word;
    next->next = NULL;

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
    char *buf;
    size_t len;
    ssize_t nread;

    head = NULL;
    buf = NULL;
    while ((nread = getline(&buf, &len, fp)) != -1) {
        char *c;

        /* Strip out trailing whitespace */
        for (c = buf; *c != '\0'; ++c) {
            if (isspace(*c)) {
                *c = '\0';
                break;
            }
        }

        /* Add the word to the list */
        /* curr takes ownership of buf */
        curr = word_list_add(prev, buf);
        if (curr == NULL) {
            /* Failed to allocate enough memory */
            if (head != NULL)
                word_list_free(head);
            return NULL;
        } else
            prev = curr;

        /* Save a pointer to the first word */
        if (head == NULL)
            head = curr;

        /* Tell getline() to allocate a new buffer for the next one */
        buf = NULL;
    }

    return head;
}
