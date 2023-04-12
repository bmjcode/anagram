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

#ifndef LETTER_POOL_H
#define LETTER_POOL_H

#ifdef __cplusplus
extern "C" {
#endif


#include <stdbool.h>

/* This assumes the alphabet is encoded in one contiguous block */
#define POOL_SIZE  26
#define POOL_START 'a'
#define POOL_STOP  'z'

/*
 * Return whether 'letter' is part of the pool's alphabet.
 */
#define pool_in_alphabet(letter) \
        ((letter) >= POOL_START && (letter) <= POOL_STOP)

/*
 * Return whether there is at least one of 'letter' in the pool.
 */
#define pool_contains(pool, letter) \
        ((pool)[(letter) - POOL_START] >= 1)

/*
 * Add alphabetic characters in 'letters' to the pool.
 */
void pool_add(unsigned int *pool, const char *letters);

/*
 * Subtract alphabetic characters in 'letters' from the pool.
 *
 * Use pool_can_spell() to make sure there are enough of each letter
 * in the pool before calling this.
 */
void pool_subtract(unsigned int *pool, const char *letters);

/*
 * Returns whether there are enough letters in the pool to spell
 * the specified word or phrase. Spaces and punctuation are ignored.
 */
bool pool_can_spell(unsigned int *pool, const char *phrase);

/*
 * Reset a pool's letter count to zero.
 *
 * Always call this to initialize a new pool before adding letters.
 */
void pool_reset(unsigned int *pool);

/*
 * Return whether the letter pool is empty (all counts are zero).
 */
bool pool_is_empty(unsigned int *pool);

/*
 * Make a copy of a letter pool.
 *
 * This does not do any bounds checking. The caller is responsible
 * to ensure both src and dst are large enough to hold the pool.
 */
void pool_copy(unsigned int *src, unsigned int *dst);

/*
 * Print the contents of the pool to stdout. Intended for debugging.
 */
void pool_print(unsigned int *pool);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LETTER_POOL_H */
