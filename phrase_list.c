/*
 * Functions for working with phrase lists.
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
#include <unistd.h>

#include "phrase_list.h"

size_t
phrase_filter_default(char *candidate, pool_t *letter_pool, void *user_data)
{
    const char *c;
    size_t length, letter_count;
    pool_t antipool[POOL_SIZE];

    if (candidate == NULL)
        return 0;

    pool_reset(antipool);

    /* We need the total length for memory allocation, and the letter count
     * to determine if this is a usable phrase. */
    length = 0;
    letter_count = 0;

    /* By avoiding library functions and doing all the counting ourselves,
     * we can do everything we need in a single pass. */
    for (c = candidate; !phrase_terminator(*c); ++c) {
        if (pool_in_alphabet(*c)) {
            if (letter_pool != NULL) {
                pool_add_letter(antipool, *c);
                if (pool_count(antipool, *c) > pool_count(letter_pool, *c))
                    return 0;
            }
            ++letter_count;
        } else if (phrase_cannot_include(*c))
            return 0;
        ++length;
    }

    /* Reject phrases with too few letters. */
    if ((letter_count == 0) || (length - letter_count > letter_count))
        return 0;

    /* If we made it this far, accept the phrase. */
    return length;
}

struct phrase_list *
phrase_list_add(struct phrase_list *prev,
                const char *phrase, size_t length, size_t *count)
{
    struct phrase_list *next;
    size_t i;

    next = NULL;
    if ((phrase == NULL) || (length == 0))
        goto failsafe;

    next = malloc(sizeof(struct phrase_list));
    if (next == NULL)
        goto failsafe;
    next->phrase = malloc((length + 1) * sizeof(char));
    if (next->phrase == NULL)
        goto failsafe;
    next->length = length;
    next->next = NULL;

    /* We can't rely on a safe function like strlcpy() being available,
     * so we'll just copy the string ourselves. */
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
    return phrase_list_read_filtered(prev, fp, count, letter_pool,
                                     phrase_filter_default, NULL);
}

struct phrase_list *
phrase_list_read_filtered(struct phrase_list *prev,
                          FILE *fp,
                          size_t *count,
                          pool_t *letter_pool,
                          phrase_filter_cb phrase_filter,
                          void *user_data)
{
    struct phrase_list *head, *curr;
    char buf[64]; /* that should be long enough for most words */
    size_t length;

    if (fp == NULL)
        return NULL;
    else if (phrase_filter == NULL)
        phrase_filter = phrase_filter_default;

    head = NULL;
    while (fgets(buf, sizeof(buf), fp) != NULL) {
        length = phrase_filter(buf, letter_pool, user_data);
        if (length == 0)
            continue;

        curr = phrase_list_add(prev, buf, length, count);
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

bool
phrase_list_write(struct phrase_list *head, FILE *fp)
{
    struct phrase_list *curr;
    size_t bytes_written;

    if ((head == NULL) || (fp == NULL))
        return false;

    for (curr = head; curr != NULL; curr = curr->next) {
        bytes_written = fprintf(fp, "%s\n", curr->phrase);
        if (bytes_written != curr->length + 1)
            return false;
    }
    return true;
}

/* This should always return a valid path so we can display a useful error
 * message if the list does not exist. Do not use /usr/share/dict/words
 * because it's not portable to non-Unix systems, and its presence is not
 * guaranteed even on those. (For example, my preferred Linux distribution
 * does not include it in a default installation.) */
const char *
phrase_list_default(void)
{
    /* FIXME: There has to be a better way to locate this file. */
    return "web2.txt";
}

char *
phrase_first_word(struct phrase_list *item, size_t *length)
{
    char *c;

    if ((item == NULL) || (length == NULL))
        return NULL;

    *length = 0;
    for (c = item->phrase;
         !phrase_delimiter(*c);
         ++c)
        ++*length;
    return item->phrase;
}

char *
phrase_last_word(struct phrase_list *item, size_t *length)
{
    char *c;

    if ((item == NULL) || (length == NULL))
        return NULL;

    *length = 0;
    for (c = item->phrase + item->length - 1;
         !phrase_delimiter(*c);
         --c)
        ++*length;
    return c + 1; /* we stopped on the delimiter */
}
