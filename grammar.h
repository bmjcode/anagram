/*
 * Functions to implement trivial English grammar rules.
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

#ifndef GRAMMAR_H
#define GRAMMAR_H

#include <stdbool.h>

#include "phrase_list.h"

/* Option flags for grammar_allows() */
#define GA_NO_REPEATS   (1 << 0)    /* don't repeat the last word */

/*
 * Return whether the next phrase should not follow the previous one.
 *
 * This is not a comprehensive grammar check. Its sole purpose is to block
 * obviously invalid combinations like "in the to be" that we can recognize
 * with simple hard-coded rules. Everything else is allowed, whether it is
 * actually grammatically correct or not.
 *
 * I mostly did this to see if it would make the Qwantzle solver any more
 * useful. Of course, that does raise the question of whether the Qwantzle
 * solver was ever useful in the first place.
 */
bool grammar_prohibits(char *prev, size_t prev_length,
                       char *next, size_t next_length,
                       unsigned int options);

#endif /* GRAMMAR_H */
