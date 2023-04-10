/*
 * Find words in a given list spellable with a limited pool of letters.
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
 * Usage: ./word_search alphabet /path/to/word/list
 *
 * For example, to find English language isograms (words spelled using
 * each of their letters exactly once), you could try:
 * ./word_search abcdefghijklmnopqrstuvwxyz /usr/share/dict/words
 *
 * The formatting of the word list is one word per line, case-sensitive.
 * Matching words are printed to stdout.
 */

#include <stdio.h>
#include <stdlib.h>

#include "letter_pool.h"
#include "util.h"
#include "word_list.h"

int
main(int argc, char **argv)
{
    FILE *fp;
    struct word_list *word_list, *curr;
    char *letters, *list_path;
    unsigned int pool[POOL_SIZE];

    pool_reset(pool);
    if (argc < 3) {
        fprintf(stderr,
                "Usage: %s alphabet /path/to/word/list\n",
                argv[0]);
        return 1;
    }

    letters = argv[1];
    pool_add(pool, letters);

    list_path = argv[2];
    fp = fopen(list_path, "r");
    if (fp == NULL) {
        fprintf(stderr,
                "Failed to open: %s\n",
                list_path);
        return 1;
    }

    /* Read words from the list */
    word_list = word_list_read(NULL, fp);
    if (word_list == NULL) {
        fprintf(stderr,
                "Failed to read word list: %s\n",
                list_path);
        return 1;
    }
    fclose(fp);

    /* Iterate through the list looking for spellable words */
    for (curr = word_list; curr != NULL; curr = curr->next) {
        if (pool_can_spell(pool, curr->word))
            printf("%s\n", curr->word);
    }

    /* Free memory and return success */
    word_list_free(word_list);
    return 0;
}
