/*
 * Find anagrams of a given word.
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
 * Usage: ./anagram_search word /path/to/word/list
 *
 * For example, to find anagrams of the word "leprechaun", you could try:
 * ./anagram_search leprechaun /usr/share/dict/words
 *
 * The formatting of the word list is one word per line, case-sensitive.
 * Found anagrams are printed to stdout.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "letter_pool.h"
#include "util.h"
#include "word_list.h"

/*
 * Make a "sentence" using words formed from letters in the pool.
 * For our purposes a sentence is any combination of one or more words
 * separated by spaces.
 */
static void make_sentence(struct word_list *word_list,
                          unsigned int *pool,
                          const char *prev_sentence);

void
make_sentence(struct word_list *word_list,
              unsigned int *pool,
              const char *prev_sentence)
{
    struct word_list *curr;

    if ((word_list == NULL) || (pool == NULL))
        return;

    if (pool_is_empty(pool) && (prev_sentence != NULL)) {
        printf("%s\n", prev_sentence);
        return;
    }

    /* Iterate through the word list */
    for (curr = word_list; curr != NULL; curr = curr->next) {
        char *new_sentence, *n, *w;
        size_t s_len;

        if (curr->word == NULL)
            continue;

        /* Make sure we have enough letters to spell this word */
        if (!pool_can_spell(pool, curr->word))
            continue;

        /* Remove this word's letters from the pool */
        pool_subtract(pool, curr->word);

        /* Calculate how much memory to allocate for the new sentence */
        s_len = strlen(curr->word) + 1; /* extra for the trailing '\0' */
        if (prev_sentence != NULL)
            s_len += strlen(prev_sentence) + 1; /* extra for a space */

        /* Add this word to our sentence */
        /* Inlining this may save a few microseconds over string.h */
        new_sentence = malloc(s_len * sizeof(char));
        n = new_sentence;
        if (prev_sentence != NULL) {
            const char *p;
            p = prev_sentence;
            while (*p != '\0')
                *n++ = *p++;
            *n++ = ' ';
        }
        w = curr->word;
        while (*w != '\0')
            *n++ = *w++;
        *n = '\0';

        /* Call this function recursively to extend the sentence */
        make_sentence(word_list, pool, new_sentence);
        free(new_sentence);

        /* Restore the pool and our previous position in the word list */
        pool_add(pool, curr->word);
    }
}

int
main(int argc, char **argv)
{
    FILE *fp;
    struct word_list *word_list;
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

    /* Search for valid sentences */
    make_sentence(word_list, pool, NULL);

    /* Free memory and return success */
    word_list_free(word_list);
    return 0;
}
