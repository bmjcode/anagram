/*
 * Functions for working with an in-memory word list.
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

#ifndef WORD_LIST_H
#define WORD_LIST_H

#ifdef __cplusplus
extern "C" {
#endif


struct word_list {
    char *word;
    struct word_list *next;
};


/*
 * Add a word to the list.
 *
 * If prev is NULL, this starts a new list.
 * The struct word_list takes responsibility for managing 'word''s memory.
 * Returns a pointer to the newly added word.
 */
struct word_list *word_list_add(struct word_list *prev, char *word);

/*
 * Free memory used by a word list.
 */
void word_list_free(struct word_list *first);

/*
 * Read a word list from a file.
 *
 * The file pointed to by 'fp' should be opened in mode "r".
 * Returns a pointer to the first item in the list.
 */
struct word_list *word_list_read(struct word_list *prev, FILE *fp);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* WORD_LIST_H */