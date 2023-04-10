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

/* Length of the line buffer for word_list_read() */
/* This should fit the longest word we expect to encounter */
#define BUF_SIZE 64

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

    head = NULL;
    while (!feof(fp)) {
        char *c;

        /* Allocate memory */
        if ((buf = malloc(BUF_SIZE * sizeof(char))) == NULL)
            goto failsafe;

        /* Read the next line from the file */
        if (fgets(buf, BUF_SIZE, fp) == NULL) {
            /* Note we expect this to fail at EOF */
            if (buf != NULL)
                free(buf);
            break;
        }

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
        if (curr == NULL)
            goto failsafe;
        else
            prev = curr;

        /* Save a pointer to the first word */
        if (head == NULL)
            head = curr;

        /* Indicate the buffer is no longer our problem */
        buf = NULL;
    }

    return head;

failsafe:
    if (head != NULL)
        word_list_free(head);

    if (buf != NULL)
        free(buf);

    return NULL;
}
