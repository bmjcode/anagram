/*
 * Indicate whether a phrase is spellable using the letters in the pool.
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

/*
 * Usage: ./is_spellable alphabet word [word ...]
 *
 * Returns 0 if all words can be spelled, 1 otherwise.
 */

#include <stdio.h>

#include "letter_pool.h"

int
main(int argc, char **argv)
{
    int i;
    char *letters, *word;
    unsigned int pool[POOL_SIZE];

    pool_reset(pool);
    if (argc < 3) {
        fprintf(stderr,
                "Usage: %s alphabet word [word ...]\n",
                argv[0]);
        return 1;
    }

    letters = argv[1];
    pool_add(pool, letters);

    for (i = 2; i < argc; ++i) {
        word = argv[i];
        if (pool_can_spell(pool, word))
            pool_subtract(pool, word);
        else
            return 1;
    }

    return 0;
}
