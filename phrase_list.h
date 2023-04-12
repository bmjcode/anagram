/*
 * Functions for working with an in-memory phrase list.
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
 * For our purposes a "phrase" is any combination of one or more words
 * joined by spaces and/or punctuation.
 */

#ifndef PHRASE_LIST_H
#define PHRASE_LIST_H

struct phrase_list {
    char *phrase;
    size_t length;
    struct phrase_list *next;
};

/*
 * Add a phrase to the list.
 *
 * If prev is NULL, this starts a new list.
 * Returns a pointer to the newly added phrase.
 */
struct phrase_list *phrase_list_add(struct phrase_list *prev,
                                    const char *phrase, size_t *count);

/*
 * Free memory used by a phrase list.
 */
void phrase_list_free(struct phrase_list *first);

/*
 * Read a phrase list from a file.
 *
 * The file pointed to by 'fp' should be opened in mode "r".
 * Returns a pointer to the first item in the list.
 */
struct phrase_list *phrase_list_read(struct phrase_list *prev,
                                     FILE *fp,
                                     size_t *count);

#endif /* PHRASE_LIST_H */
