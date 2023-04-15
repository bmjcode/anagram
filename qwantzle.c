/*
 * Solve the anacryptogram from Dinosaur Comics #1663 (aka the Qwantzle).
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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "letter_pool.h"
#include "phrase_list.h"
#include "sentence.h"

static bool qwantzle_check(struct sentence_info *si,
                           struct phrase_list *newest_phrase);
static void usage(FILE *stream, char *prog_name);

bool
qwantzle_check(struct sentence_info *si,
               struct phrase_list *newest_phrase)
{
    /* The final letter is "w" */
    if (pool_is_empty(si->pool)) {
        if (newest_phrase->phrase[newest_phrase->length - 1] != 'w')
            return false;
    } else {
        if (!pool_contains(si->pool, 'w'))
            return false;
    }
    return true;
}

void
usage(FILE *stream, char *prog_name)
{
    fprintf(stream,
            "Solve the anacryptogram from Dinosaur Comics #1663.\n"
            "Usage: %s [-h] [-f] [-l PATH] [-t NUM]\n"
            "  -h       Display this help message and exit\n"
            "  -f       Filter mode (read phrase list from stdin)\n"
            "  -l PATH  Override the default phrase list\n"
            "  -t NUM   Start the specified number of threads (default: 1)\n",
            prog_name);
}

int
main(int argc, char **argv)
{
    FILE *fp;
    struct sentence_info si;
    const char *list_path;
    pool_t pool[POOL_SIZE];
    int opt;
    unsigned short num_threads;

    pool_reset(pool);
    sentence_info_init(&si);
    si.pool = pool;
    si.check_cb = qwantzle_check;

    /* The first word of the solution is "I" */
    si.sentence = "I";
    si.length = 1;

    /* Add the remaining letters */
    pool_add(si.pool,
             "ttttttttttttooooooooooeeeeeeeeaaaaaaallllllnnnnnn"
             "uuuuuuiiiiisssssdddddhhhhhyyyyyIIrrrfffbbwwkcmvg");

    fp = NULL;
    list_path = NULL;
    num_threads = 1;

    while ((opt = getopt(argc, argv, "hfl:t:")) != -1) {
        switch (opt) {
            case 'h':
                /* Display help and exit */
                (void)num_threads;
                usage(stdout, argv[0]);
                return 0;
            case 'f':
                /* Filter mode */
                fp = stdin;
                break;
            case 'l':
                /* Override the default phrase list */
                list_path = optarg;
                break;
            case 't':
                /* Set the number of threads */
                num_threads = strtoul(optarg, NULL, 0);
                break;
        }
    }

    if (list_path == NULL)
        list_path = "searchlist.txt";
    else if (strcmp(list_path, "-") == 0)
        fp = stdin;

    if (fp == NULL) {
        fp = fopen(list_path, "r");
        if (fp == NULL) {
            fprintf(stderr,
                    "Failed to open: %s\n",
                    list_path);
            return 1;
        }
    }
    si.phrase_list = phrase_list_read(NULL, fp, &si.phrase_count, pool);
    fclose(fp);

    if (si.phrase_list == NULL) {
        fprintf(stderr,
                "Failed to read phrase list: %s\n",
                list_path);
        return 1;
    }

    /* Search for valid sentences */
    sentence_build_threaded(&si, num_threads);

    phrase_list_free(si.phrase_list);
    return 0;
}
