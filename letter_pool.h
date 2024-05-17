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

#include <stdbool.h>

/*
 * This implementation assumes a character set with certain ASCII-like
 * properties -- namely, that the alphabet is encoded in one contiguous
 * block, with uppercase sorted first. This lets us calculate pool_t
 * array indices using simple pointer arithmetic, which provides a small
 * speed boost in functions like sentence_build() that use pools heavily.
 *
 * I believe this is a safe assumption for most people since exceptions
 * like EBCDIC are, to my knowledge, uncommon. Extending these functions
 * to other alphabets and encodings is left as an exercise to the reader.
 */

/* A letter pool is an array of pool_t variables counting how many times each
 * letter of the alphabet is used in a phrase. Use POOL_SIZE as its length. */
typedef size_t pool_t;

/* These are implementation details. Do not use them in your own code.
 * They are defined first only because we need them to define POOL_SIZE. */
#define POOL_START 'A'
#define POOL_STOP  'z'

/* This is the correct length for a pool_t array. */
#define POOL_SIZE  (POOL_STOP - POOL_START + 1)

/*
 * Return whether 'letter' is part of the pool's alphabet.
 */
#define pool_in_alphabet(letter) \
        ((letter) >= POOL_START && (letter) <= POOL_STOP)

/*
 * Returns the number of a given letter in the pool.
 */
#define pool_count(pool, letter) \
        ((pool)[(letter) - POOL_START])

/*
 * Return whether there is at least one of 'letter' in the pool.
 */
#define pool_contains(pool, letter) \
        (pool_count(pool, letter) >= 1)

/*
 * Returns the total number of (non-unique) letters in the pool.
 */
size_t pool_count_all(pool_t *pool);

/*
 * Add alphabetic characters in 'letters' to the pool.
 */
void pool_add(pool_t *pool, const char *letters);

/*
 * Subtract alphabetic characters in 'letters' from the pool.
 *
 * Use pool_can_spell() to make sure there are enough of each letter
 * in the pool before calling this.
 */
void pool_subtract(pool_t *pool, const char *letters);

/*
 * Returns whether there are enough letters in the pool to spell
 * the specified word or phrase. Spaces and punctuation are ignored.
 */
bool pool_can_spell(pool_t *pool, const char *phrase);

/*
 * Reset a pool's letter count to zero.
 *
 * Always call this to initialize a new pool before adding letters.
 */
void pool_reset(pool_t *pool);

/*
 * Return whether the letter pool is empty (all counts are zero).
 */
#define pool_is_empty(pool) \
        (pool_count_all(pool) == 0)

/*
 * Make a copy of a letter pool.
 *
 * This does not do any bounds checking. The caller is responsible
 * to ensure both src and dst are large enough to hold the pool.
 */
void pool_copy(pool_t *src, pool_t *dst);

#endif /* LETTER_POOL_H */
