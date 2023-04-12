/*
 * Functions for building a sentence from a phrase list.
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

#ifndef SENTENCE_H
#define SENTENCE_H

#include "letter_pool.h"
#include "phrase_list.h"

/*
 * Structure representing the state of make_sentence().
 */
struct sentence_info {
    struct phrase_list *phrase_list;
    size_t phrase_count;
    unsigned int *pool;
    char *sentence;
    size_t length;
};

/*
 * Initialize a sentence_info structure.
 */
void sentence_info_init(struct sentence_info *si);

/*
 * Build a "sentence" using phrases formed from letters in the pool.
 * For our purposes a sentence is any combination of one or more phrases
 * separated by spaces.
 */
void sentence_build(struct sentence_info *si);

#endif /* SENTENCE_H */
