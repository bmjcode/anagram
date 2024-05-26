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
#include <string.h>
#include <unistd.h>

#include "phrase_list.h"

static int phrase_compare(const void *left, const void *right);

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
phrase_list_sort(struct phrase_list *orig, size_t count)
{
    struct phrase_list *first, *curr, **items;
    size_t i;

    if ((orig == NULL) || (count == 0))
        return NULL;

    first = orig; /* for now */

    items = malloc(count * sizeof(struct phrase_list*));
    if (items == NULL)
        goto cleanup;

    for (i = 0, curr = first;
         (i < count) && (curr != NULL);
         curr = curr->next, ++i)
        items[i] = curr;

    qsort(items, count, sizeof(struct phrase_list*), phrase_compare);

    first = items[0];
    for (i = 0; i + 1 < count; ++i)
        items[i]->next = items[i + 1];
    items[count - 1]->next = NULL;

cleanup:
    if (items != NULL)
        free(items);
    return first;
}

void
phrase_list_uniq(struct phrase_list *first, size_t *count)
{
    struct phrase_list *curr, *skipped;

    for (curr = first; curr != NULL; curr = curr->next) {
        while ((curr->next != NULL)
               && (curr->next->length == curr->length)
               && (strcmp(curr->next->phrase, curr->phrase) == 0)) {
            /* The next item is a duplicate */
            skipped = curr->next;
            curr->next = skipped->next;

            if (skipped->phrase != NULL)
                free(skipped->phrase);
            free(skipped);

            if (count != NULL)
                --*count;
        }
    }
}

struct phrase_list *
phrase_list_normalize(struct phrase_list *first, size_t *count)
{
    struct phrase_list *prev, *curr, *next, *candidate;
    size_t i;

    if (first == NULL)
        return NULL;

    phrase_list_uniq(first, count);

    /* We're relying on the assumption that uppercase sorts first, which
     * makes the last instance of a given phrase the most-lowercase version
     * and therefore the one to keep. */
    prev = NULL;
    curr = first;
    while (!((curr == NULL) || (curr->phrase == NULL))) {
        char lp[curr->length + 1]; /* lowercase phrase */

        /* Stop when we get to words that are already lowercase */
        if (islower((unsigned char) curr->phrase[0]))
            break;

        /* Use lowercase for case-insensitive comparisons */
        for (i = 0; i < curr->length; ++i)
            lp[i] = tolower((unsigned char) curr->phrase[i]);
        lp[curr->length] = '\0';

        next = curr->next;
        for (candidate = next;
             !((candidate == NULL) || (candidate->phrase == NULL));
             candidate = candidate->next) {
            /* Did we miss it? */
            if (candidate->phrase[0] > lp[0])
                goto no_more_candidates;

            /* If they're not the same length, they're not the same word */
            if (candidate->length != curr->length)
                goto next_candidate;

            /* If their letters don't match, they're not the same word */
            for (i = 0; i < candidate->length; ++i) {
                if (tolower((unsigned char) candidate->phrase[i]) != lp[i])
                    goto next_candidate;
            }

            /* If we haven't bailed out yet, they must be the same word,
             * so discard the more-uppercase version */
            if (prev == NULL)
                first = next;
            else
                prev->next = next;

            free(curr->phrase);
            free(curr);
            curr = NULL;

            if (count != NULL)
                --*count;
            break;

next_candidate:
            continue; /* avoid "label at end of compound statement" errors */
        }

no_more_candidates:
        if (curr != NULL)
            prev = curr;
        curr = next;
    }
    return first;
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
phrase_first_word(char *phrase, size_t phrase_length, size_t *word_length)
{
    char *c;

    if ((phrase == NULL)
        || (phrase_length == 0)
        || (word_length == NULL))
        return NULL;

    *word_length = 0;
    for (c = phrase;
         !phrase_delimiter(*c);
         ++c)
        ++*word_length;
    return phrase;
}

char *
phrase_last_word(char *phrase, size_t phrase_length, size_t *word_length)
{
    char *c;

    if ((phrase == NULL)
        || (phrase_length == 0)
        || (word_length == NULL))
        return NULL;

    /* Find the last non-delimiter in the phrase */
    c = phrase + phrase_length - 1;
    while (phrase_delimiter(*c))
        --c;

    *word_length = 0;
    for (;
         !phrase_delimiter(*c);
         --c)
        ++*word_length;
    return c + 1; /* we stopped on the delimiter */
}

/*
 * Compare two struct phrase_list* items for sorting with qsort().
 */
int
phrase_compare(const void *left_, const void *right_)
{
    const struct phrase_list
        *left = *(struct phrase_list **)(left_),
        *right = *(struct phrase_list **)(right_);

    if ((left == NULL) || (right == NULL))
        return 0; /* vacuous */
    else
        return strcmp(left->phrase, right->phrase);
}
