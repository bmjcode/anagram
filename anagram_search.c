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
#include "word_list.h"

/*
 * Structure representing the state of make_sentence().
 */
struct sentence_info {
    struct word_list *word_list;
    size_t word_count;
    unsigned int *pool;
    char *sentence;
    size_t length;
};

/*
 * Initialize a sentence_info structure.
 */
static void sentence_info_init(struct sentence_info *si);

/*
 * Make a "sentence" using words formed from letters in the pool.
 * For our purposes a sentence is any combination of one or more words
 * separated by spaces.
 */
static void make_sentence(struct sentence_info *si);

void
sentence_info_init(struct sentence_info *si)
{
    if (si == NULL)
        return;

    si->word_list = NULL;
    si->word_count = 0;
    si->pool = NULL;
    si->sentence = NULL;
    si->length = 0;
}

void
make_sentence(struct sentence_info *si)
{
    struct word_list *curr;

    if ((si == NULL) || (si->word_list == NULL) || (si->pool == NULL))
        return;

    if (pool_is_empty(si->pool) && (si->sentence != NULL)) {
        /* We've completed a sentence */
        printf("%s\n", si->sentence);
        return;
    }

    for (curr = si->word_list; curr != NULL; curr = curr->next) {
        struct sentence_info nsi;
        char *n, *w;

        if (curr->word == NULL)
            break; /* best to assume something's really gone wrong here */

        if (!pool_can_spell(si->pool, curr->word))
            continue;

        sentence_info_init(&nsi);
        nsi.word_list = si->word_list;
        nsi.pool = si->pool;

        if (si->sentence == NULL)
            nsi.length = curr->length;
        else
            nsi.length = si->length + curr->length + 1; /* add a space */

        nsi.sentence = malloc((nsi.length + 1) * sizeof(char));
        if (nsi.sentence == NULL)
            break;

        /* Inlining this avoids strcat()'s excessive seeking */
        /* This isn't overflow-safe but neither is anything else here */
        n = nsi.sentence;
        if (si->sentence != NULL) {
            char *p;
            p = si->sentence;
            while (*p != '\0')
                *n++ = *p++;
            *n++ = ' ';
        }
        w = curr->word;
        while (*w != '\0')
            *n++ = *w++;
        *n = '\0';

        /* Remove this word's letters from the pool */
        pool_subtract(si->pool, curr->word);

        /* Extend the sentence with our new phrase */
        make_sentence(&nsi);
        free(nsi.sentence);

        /* Restore the pool for the next cycle */
        pool_add(si->pool, curr->word);
    }
}

int
main(int argc, char **argv)
{
    FILE *fp;
    struct sentence_info si;
    char *letters, *list_path;
    unsigned int pool[POOL_SIZE];

    sentence_info_init(&si);
    si.pool = pool;

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

    si.word_list = word_list_read(NULL, fp, &si.word_count);
    fclose(fp);
    if (si.word_list == NULL) {
        fprintf(stderr,
                "Failed to read word list: %s\n",
                list_path);
        return 1;
    }

    /* Search for valid sentences */
    make_sentence(&si);

    word_list_free(si.word_list);
    return 0;
}
