/*
 * Functions to find words spellable with a limited pool of letters.
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

#include "letter_pool.h"

void
pool_add(pool_t *pool, const char *letters)
{
    if ((pool == NULL) || (letters == NULL))
        return;

    while (*letters != '\0') {
        if (pool_in_alphabet(*letters))
            ++pool[*letters - POOL_START];
        ++letters;
    }
}

void
pool_subtract(pool_t *pool, const char *letters)
{
    if ((pool == NULL) || (letters == NULL))
        return;

    while (*letters != '\0') {
        if (pool_in_alphabet(*letters))
            --pool[*letters - POOL_START];
        ++letters;
    }
}

bool
pool_can_spell(pool_t *pool, const char *phrase)
{
    const char *c;
    size_t pos;
    pool_t letter_count[POOL_SIZE];

    if ((pool == NULL) || (phrase == NULL))
        return false;

    pool_reset(letter_count);

    c = phrase;
    while (*c != '\0') {
        if (pool_in_alphabet(*c)) {
            pos = *c - POOL_START;
            if (++letter_count[pos] > pool[pos])
                return false;
        } else if (!(ispunct(*c) || isspace(*c)))
            return false;
        ++c;
    }
    return true;
}

void
pool_reset(pool_t *pool)
{
    size_t i;

    if (pool == NULL)
        return;

    for (i = 0; i < POOL_SIZE; ++i)
        pool[i] = 0;
}

bool
pool_is_empty(pool_t *pool)
{
    size_t i;

    if (pool == NULL)
        return false; /* vacuous since there is no pool */

    for (i = 0; i < POOL_SIZE; ++i) {
        if (pool[i] > 0)
            return false;
    }
    return true;
}

void
pool_copy(pool_t *src, pool_t *dst)
{
    size_t i;

    if ((src == NULL) || (dst == NULL))
        return;

    for (i = 0; i < POOL_SIZE; ++i)
        dst[i] = src[i];
}
