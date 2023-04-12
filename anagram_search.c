/*
 * Find anagrams of a given word or phrase.
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
 * Usage: ./anagram_search phrase /path/to/phrase/list
 *
 * For example, to find anagrams of the word "leprechaun", you could try:
 * ./anagram_search leprechaun /usr/share/dict/words
 *
 * The formatting of the phrase list is one per line, case-sensitive.
 * Found anagrams are printed to stdout.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "letter_pool.h"
#include "phrase_list.h"
#include "sentence.h"

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
                "Usage: %s alphabet /path/to/phrase/list\n",
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

    si.phrase_list = phrase_list_read(NULL, fp, &si.phrase_count);
    fclose(fp);
    if (si.phrase_list == NULL) {
        fprintf(stderr,
                "Failed to read phrase list: %s\n",
                list_path);
        return 1;
    }

    /* Search for valid sentences */
    sentence_build(&si);

    phrase_list_free(si.phrase_list);
    return 0;
}
