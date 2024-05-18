/*
 * Functions for working with phrase lists.
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
 * Phrases are the basic unit of sentence construction. For our purposes,
 * a "phrase" consists of one or more words (in the everyday sense) joined
 * by spaces. Phrases are always considered for addition to a sentence as
 * a whole, so for their constituent words to be considered individually
 * they must also be listed that way. Phrases may include punctuation, to
 * allow for contractions, but not digits.
 */

#ifndef PHRASE_LIST_H
#define PHRASE_LIST_H

#include <ctype.h>          /* for is*() */
#include <stdio.h>          /* for FILE */

#include "letter_pool.h"    /* for pool_t */

struct phrase_list {
    char *phrase;
    size_t length;
    struct phrase_list *next;
};

/*
 * Prototype for a phrase filter function.
 *
 * The phrase filter determines if a candidate phrase is usable. Most
 * applications should probably stick with the default filter, but you can
 * can define your own if you need to implement more complex rules like only
 * accepting phrases of a certain length.
 *
 * If the candidate phrase is acceptable, this should return its length
 * excluding the trailing '\0' a la strlen(). Otherwise, it should return 0.
 */
typedef size_t (*phrase_filter_cb)(char *candidate,
                                   pool_t *letter_pool,
                                   void *user_data);

/*
 * The default phrase filter.
 *
 * This checks that phrases contain at least one letter and no digits.
 * If a letter pool is specified, it confirms that the phrase can be
 * spelled using the pool letters. It allows spaces and punctuation so
 * long as they make up no more than half the characters.
 */
size_t phrase_filter_default(char *candidate,
                             pool_t *letter_pool,
                             void *user_data);

/*
 * Add a phrase to a list.
 *
 * If prev is NULL, this starts a new list.
 * Returns a pointer to the newly added phrase.
 */
struct phrase_list *phrase_list_add(struct phrase_list *prev,
                                    const char *phrase, size_t length,
                                    size_t *count);

/*
 * Free memory used by a phrase list.
 */
void phrase_list_free(struct phrase_list *first);

/*
 * Read a phrase list from a file.
 *
 * The file pointed to by 'fp' should be opened in mode "r".
 * The number of phrases is stored in 'count'.
 *
 * If 'letter_pool' is provided, only words spellable using the letters
 * in the pool will be included in the list. This prevents us from
 * considering phrases we can never use -- a significant optimization.
 *
 * Returns a pointer to the first item in the list.
 */
struct phrase_list *phrase_list_read(struct phrase_list *prev,
                                     FILE *fp,
                                     size_t *count,
                                     pool_t *letter_pool);

/*
 * The same as above, but using your custom phrase filter instead of
 * the default.
 *
 * The 'user_data' parameter is arbitrary data passed directly to the
 * filter function.
 */
struct phrase_list *phrase_list_read_filtered(struct phrase_list *prev,
                                              FILE *fp,
                                              size_t *count,
                                              pool_t *letter_pool,
                                              phrase_filter_cb phrase_filter,
                                              void *user_data);

/*
 * Write a phrase list to a file.
 *
 * The file pointed to by 'fp' should be opened in mode "w".
 *
 * Returns true if the list was written successfully, false otherwise.
 */
bool phrase_list_write(struct phrase_list *head, FILE *fp);

/*
 * Return the path to our default phrase list.
 *
 * The return value is a static string. Don't free it!
 */
const char *phrase_list_default(void);

/*
 * Get the position and length of the first word in a phrase.
 *
 * The return value is a pointer to the first character of the word in
 * item->phrase. Do not free it. The caller should use the 'length' value
 * to ignore subsequent words in phrases with multiple words.
 */
char *phrase_first_word(struct phrase_list *item, size_t *length);

/*
 * Get the position and length of the last word in a phrase.
 *
 * The return value is a pointer to the first character of the word in
 * item->phrase. Do not free it.
 */
char *phrase_last_word(struct phrase_list *item, size_t *length);

/*
 * Use this in your phrase filter to identify non-alphabetic characters
 * that cannot be included in a phrase. Phrases that do contain such
 * characters should be rejected immediately.
 */
#define phrase_cannot_include(c) \
        (iscntrl(c) || !(((c) == ' ') || ispunct(c)))

/*
 * Use this in your phrase filter to identify the end of a phrase.
 * We treat newline as a terminator to exclude it from the returned length,
 * which eliminates the unwanted one from fgets() in phrase_list_read().
 */
#define phrase_terminator(c) \
        (((c) == '\n') || ((c) == '\0'))

/*
 * Use this in your phrase filter to identify the delimiter between words.
 */
#define phrase_delimiter(c) \
        (((c) == ' ') || phrase_terminator(c))

#endif /* PHRASE_LIST_H */
