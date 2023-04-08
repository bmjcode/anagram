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

#include <stdio.h>

#include "letter_pool.h"

void
pool_add(unsigned int *pool, const char *letters)
{
    if ((pool == NULL) || (letters == NULL))
        return;

    while (*letters != '\0') {
        if (pool_in_alphabet(*letters))
            ++pool[(int)(*letters - POOL_START)];
        ++letters;
    }
}

void
pool_subtract(unsigned int *pool, const char *letters)
{
    if ((pool == NULL) || (letters == NULL))
        return;

    while (*letters != '\0') {
        if (pool_in_alphabet(*letters))
            --pool[(int)(*letters - POOL_START)];
        ++letters;
    }
}

bool
pool_can_spell(unsigned int *pool, const char *word)
{
    int pos;
    unsigned int letter_count[POOL_SIZE];

    if ((pool == NULL) || (word == NULL))
        return false;

    pool_reset(letter_count);
    while (*word != '\0') {
        if (pool_in_alphabet(*word)) {
            pos = (int)(*word - POOL_START);
            if (++letter_count[pos] > pool[pos])
                return false;
        } else
            return false;
        ++word;
    }
    return true;
}

void
pool_reset(unsigned int *pool)
{
    int i;

    if (pool == NULL)
        return;

    for (i = 0; i < POOL_SIZE; ++i)
        pool[i] = 0;
}

bool
pool_is_empty(unsigned int *pool)
{
    int i;

    if (pool == NULL)
        return false; /* vacuous since there is no pool */

    for (i = 0; i < POOL_SIZE; ++i) {
        if (pool[i] > 0)
            return false;
    }
    return true;
}

void
pool_copy(unsigned int *src, unsigned int *dst)
{
    int i;

    if ((src == NULL) || (dst == NULL))
        return;

    for (i = 0; i < POOL_SIZE; ++i)
        dst[i] = src[i];
}

void
pool_print(unsigned int *pool)
{
    int i = 0;

    if (pool == NULL)
        return;

    while (true) {
        printf("'%c': %d",
               (char)i + POOL_START,
               pool[i]);
        if (++i == POOL_SIZE) {
            printf("%s", "\n");
            break;
        } else
            printf("%s", ", ");
    }
}

#ifdef TEST_LETTER_POOL
int
main(int argc, char **argv)
{
    const char *word = "isogram";
    bool can_spell;
    unsigned int pool[POOL_SIZE];

    /* Add one instance of each letter of the English alphabet */
    pool_reset(pool);
    pool_add(pool, "abcdefghijklmnopqrstuvwxyz");
    pool_print(pool);

    can_spell = pool_can_spell(pool, word);
    printf("pool_can_spell(pool, \"%s\") = %d\n", word, can_spell);

    if (can_spell) {
        pool_subtract(pool, word);
        pool_print(pool);

        can_spell = pool_can_spell(pool, word);
        printf("pool_can_spell(pool, \"%s\") = %d\n", word, can_spell);
    }

    return 0;
}
#endif /* TEST_LETTER_POOL */
