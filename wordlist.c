/*
 * List unique words in a text document.
 * Copyright (c) 2024 Benjamin Johnson <bmjcode@gmail.com>
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
#include <stdlib.h>
#include <unistd.h>

#include "phrase_list.h"

/* Maximum line length */
#define BUF_SIZE 256

static struct phrase_list *extract_words(FILE *fp,
                                         struct phrase_list *prev,
                                         size_t *word_count);
static void usage(FILE *stream, char *prog_name);

/*
 * Compile a list of non-unique words.
 */
struct phrase_list *
extract_words(FILE *fp, struct phrase_list *prev, size_t *word_count)
{
    struct phrase_list *head, *curr;
    char buf[BUF_SIZE], *word_start;
    unsigned char *c;
    size_t word_len;

    head = NULL;
    curr = NULL;

    while (!feof(fp)) {
        /* Read in the next line */
        if (fgets(buf, BUF_SIZE, fp) == NULL)
            break;

        /* Find non-unique words */
        for (c = (unsigned char*)buf;;) {
            /* Skip initial non-alphabetic characters */
            for (;
                 !isalpha(*c);
                 ++c)
                if (*c == '\0')
                    goto next_line;

            /* Count characters in this word */
            for (word_len = 0, word_start = (char*)c;
                 !isspace(*c);
                 ++word_len, ++c) {
                if (*c == '\0')
                    goto next_line;
                else if (isdigit(*c))
                    goto next_word;
                else if (ispunct(*c) && ispunct(*(c + 1)))
                    *(c + 1) = ' '; /* consecutive punctuation marks
                                     * indicate a word boundary */
            }

            /* Trim final non-alphabetic characters */
            for (--c;
                 !isalpha(*c);
                 --word_len, --c)
                if (word_len == 0)
                    goto next_word;

            /* Add this word to our list */
            curr = phrase_list_add(prev, word_start, word_len, word_count);
            if (curr == NULL)
                goto next_word;
            else if (head == NULL)
                head = curr;
            prev = curr;

next_word:
            /* Go back to the space after the word */
            for (;
                 !isspace(*c);
                 ++c)
                if (*c == '\0')
                    goto next_line;
        }

next_line:
        continue; /* avoid "label at end of compound statement" errors */
    }

    return head;
}

void
usage(FILE *stream, char *prog_name)
{
    fprintf(stream,
            "List unique words in a text document.\n"
            "Usage: %s [-h] [/path/to/document.txt ...]\n"
            "  -h       Display this help message and exit\n",
            prog_name);
}

int
main(int argc, char **argv)
{
    FILE *fp;
    struct phrase_list *word_list, *curr;
    size_t word_count;
    int i, opt, retval = 0;

    word_list = NULL;
    word_count = 0;
    fp = NULL;

    while ((opt = getopt(argc, argv, "h")) != -1) {
        switch (opt) {
            case 'h':
                /* Display help and exit */
                usage(stdout, argv[0]);
                return 0;
        }
    }
    if (optind >= argc) {
        /* No filename specified; read words from stdin */
        word_list = extract_words(stdin, NULL, &word_count);

        if (word_list == NULL) {
            retval = 1;
            goto cleanup;
        }
    } else {
        struct phrase_list *curr = word_list;

        /* Interpret the remaining command line arguments as filenames */
        for (i = optind; i < argc; ++i) {
            fp = fopen(argv[i], "r");
            if (fp == NULL) {
                fprintf(stderr,
                        "Failed to read: %s\n",
                        argv[i]);
                retval = 1;
                goto cleanup;
            }

            /* Advance to the current last word in the list */
            while (!((curr == NULL) || (curr->next == NULL)))
                curr = curr->next;

            /* Append new words */
            curr = extract_words(fp, curr, &word_count);
            fclose(fp);

            if (curr == NULL) {
                retval = 1;
                goto cleanup;
            } else if (word_list == NULL)
                word_list = curr;
        }
    }

    word_list = phrase_list_sort(word_list, word_count);
    word_list = phrase_list_normalize(word_list, &word_count);

    for (curr = word_list; curr != NULL; curr = curr->next)
        printf("%s\n", curr->phrase);

cleanup:
    if (word_list != NULL)
        phrase_list_free(word_list);
    return retval;
}
