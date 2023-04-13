/*
 * Find words spellable using only the specified letters.
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "letter_pool.h"
#include "phrase_list.h"

static void usage(FILE *stream, char *prog_name);

void
usage(FILE *stream, char *prog_name)
{
    fprintf(stream,
            "Find words spellable using only the specified letters.\n"
            "Usage: %s [-h] [-l PATH] letters\n"
            "  -h       Display this help message and exit\n"
            "  -l PATH  Override the default phrase list\n",
            prog_name);
}

int
main(int argc, char **argv)
{
    FILE *fp;
    char buf[64];
    const char *list_path;
    pool_t pool[POOL_SIZE];
    int i, opt;

    pool_reset(pool);

    list_path = NULL;
    while ((opt = getopt(argc, argv, "hl:")) != -1) {
        switch (opt) {
            case 'h':
                /* Display help and exit */
                usage(stdout, argv[0]);
                return 0;
            case 'l':
                /* Override the default word list */
                list_path = optarg;
                break;
        }
    }

    /* The remaining command-line arguments specify the subject */
    if (optind >= argc) {
        usage(stderr, argv[0]);
        return 1;
    }
    for (i = optind; i < argc; ++i)
        pool_add(pool, argv[i]);

    /* Fall back on our included phrase list if none is specified */
    if (list_path == NULL)
        list_path = phrase_list_default();

    if ((fp = fopen(list_path, "r")) == NULL) {
        fprintf(stderr,
                "Failed to open: %s\n",
                list_path);
        return 1;
    }

    while (fgets(buf, sizeof(buf), fp) != NULL) {
        char *c = buf;
        while (*c != '\0') {
            if (isspace(*c)) {
                /* Limit one word per line */
                *c = '\0';
                break;
            }
            ++c;
        }
        if (pool_can_spell(pool, buf))
            printf("%s\n", buf);
    }

    fclose(fp);
    return 0;
}