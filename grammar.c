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

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "grammar.h"

typedef unsigned int grammar_t;

/* Parts of speech for our list of known words */
enum {
    UNCLASSIFIED,   /* any part of speech not listed below */
    ARTICLE,
    CONJUNCTION,
    PREPOSITION,
    PRONOUN,
    VERB,
};

/* Subtypes for the above */
enum {
    NO_SUBTYPE,
    /* For pronouns */
    AMBIGUOUS,
    SUBJECTIVE,
    OBJECTIVE,
    POSSESSIVE,
};

/* Structure to hold a word and its properties */
struct known_word {
    char *word;
    size_t length;
    grammar_t type;
    grammar_t subtype;
};

/* Known words and their parts of speech */
/* List all entries in lowercase so we can do case-insensitive matching. */
#define KNOWN_WORD_COUNT        89
#define MAX_KNOWN_WORD_LENGTH   10
struct known_word known_words[] = {
    /* articles */
    { "a",          1,  ARTICLE, NO_SUBTYPE },
    { "an",         2,  ARTICLE, NO_SUBTYPE },
    { "the",        3,  ARTICLE, NO_SUBTYPE },

    /* conjunctions */
    { "and",        3,  CONJUNCTION, NO_SUBTYPE },
    { "but",        3,  CONJUNCTION, NO_SUBTYPE },
    { "for",        3,  CONJUNCTION, NO_SUBTYPE },
    { "nor",        3,  CONJUNCTION, NO_SUBTYPE },
    { "or",         2,  CONJUNCTION, NO_SUBTYPE },
    { "so",         2,  CONJUNCTION, NO_SUBTYPE },
    { "yet",        3,  CONJUNCTION, NO_SUBTYPE },

    /* prepositions */
    { "about",      5,  PREPOSITION, NO_SUBTYPE },
    { "above",      5,  PREPOSITION, NO_SUBTYPE },
    { "across",     6,  PREPOSITION, NO_SUBTYPE },
    { "after",      5,  PREPOSITION, NO_SUBTYPE },
    { "against",    7,  PREPOSITION, NO_SUBTYPE },
    { "along",      5,  PREPOSITION, NO_SUBTYPE },
    { "among",      5,  PREPOSITION, NO_SUBTYPE },
    { "around",     6,  PREPOSITION, NO_SUBTYPE },
    { "as",         2,  PREPOSITION, NO_SUBTYPE },
    { "at",         2,  PREPOSITION, NO_SUBTYPE },
    { "before",     6,  PREPOSITION, NO_SUBTYPE },
    { "behind",     6,  PREPOSITION, NO_SUBTYPE },
    { "below",      5,  PREPOSITION, NO_SUBTYPE },
    { "beneath",    7,  PREPOSITION, NO_SUBTYPE },
    { "beside",     6,  PREPOSITION, NO_SUBTYPE },
    { "between",    7,  PREPOSITION, NO_SUBTYPE },
    { "beyond",     6,  PREPOSITION, NO_SUBTYPE },
    { "by",         2,  PREPOSITION, NO_SUBTYPE },
    { "despite",    7,  PREPOSITION, NO_SUBTYPE },
    { "down",       4,  PREPOSITION, NO_SUBTYPE },
    { "during",     6,  PREPOSITION, NO_SUBTYPE },
    { "except",     6,  PREPOSITION, NO_SUBTYPE },
    { "for",        3,  PREPOSITION, NO_SUBTYPE },
    { "from",       4,  PREPOSITION, NO_SUBTYPE },
    { "if",         2,  PREPOSITION, NO_SUBTYPE },
    { "in",         2,  PREPOSITION, NO_SUBTYPE },
    { "inside",     6,  PREPOSITION, NO_SUBTYPE },
    { "into",       4,  PREPOSITION, NO_SUBTYPE },
    { "like",       4,  PREPOSITION, NO_SUBTYPE },
    { "near",       4,  PREPOSITION, NO_SUBTYPE },
    { "of",         2,  PREPOSITION, NO_SUBTYPE },
    { "off",        3,  PREPOSITION, NO_SUBTYPE },
    { "on",         2,  PREPOSITION, NO_SUBTYPE },
    { "onto",       4,  PREPOSITION, NO_SUBTYPE },
    { "opposite",   8,  PREPOSITION, NO_SUBTYPE },
    { "out",        3,  PREPOSITION, NO_SUBTYPE },
    { "outside",    7,  PREPOSITION, NO_SUBTYPE },
    { "over",       4,  PREPOSITION, NO_SUBTYPE },
    { "past",       4,  PREPOSITION, NO_SUBTYPE },
    { "round",      5,  PREPOSITION, NO_SUBTYPE },
    { "since",      5,  PREPOSITION, NO_SUBTYPE },
    { "than",       4,  PREPOSITION, NO_SUBTYPE },
    { "through",    7,  PREPOSITION, NO_SUBTYPE },
    { "to",         2,  PREPOSITION, NO_SUBTYPE },
    { "towards",    7,  PREPOSITION, NO_SUBTYPE },
    { "under",      5,  PREPOSITION, NO_SUBTYPE },
    { "underneath", 10, PREPOSITION, NO_SUBTYPE },
    { "unlike",     6,  PREPOSITION, NO_SUBTYPE },
    { "until",      5,  PREPOSITION, NO_SUBTYPE },
    { "up",         2,  PREPOSITION, NO_SUBTYPE },
    { "upon",       4,  PREPOSITION, NO_SUBTYPE },
    { "via",        3,  PREPOSITION, NO_SUBTYPE },
    { "with",       4,  PREPOSITION, NO_SUBTYPE },
    { "within",     6,  PREPOSITION, NO_SUBTYPE },
    { "without",    7,  PREPOSITION, NO_SUBTYPE },

    /* pronouns */
    { "you",        3,  PRONOUN, AMBIGUOUS },
    { "her",        3,  PRONOUN, AMBIGUOUS },
    { "it",         2,  PRONOUN, AMBIGUOUS },
    { "i",          1,  PRONOUN, SUBJECTIVE },
    { "he",         2,  PRONOUN, SUBJECTIVE },
    { "she",        3,  PRONOUN, SUBJECTIVE },
    { "we",         2,  PRONOUN, SUBJECTIVE },
    { "they",       4,  PRONOUN, SUBJECTIVE },
    { "me",         2,  PRONOUN, OBJECTIVE },
    { "him",        3,  PRONOUN, OBJECTIVE },
    { "us",         2,  PRONOUN, OBJECTIVE },
    { "them",       4,  PRONOUN, OBJECTIVE },
    { "my",         2,  PRONOUN, POSSESSIVE },
    { "your",       4,  PRONOUN, POSSESSIVE },
    { "his",        3,  PRONOUN, POSSESSIVE },
    { "its",        3,  PRONOUN, POSSESSIVE },
    { "our",        3,  PRONOUN, POSSESSIVE },
    { "their",      5,  PRONOUN, POSSESSIVE },

    /* verbs (just forms of "to be") */
    { "be",         2,  VERB, NO_SUBTYPE },
    { "am",         2,  VERB, NO_SUBTYPE },
    { "is",         2,  VERB, NO_SUBTYPE },
    { "are",        3,  VERB, NO_SUBTYPE },
    { "was",        3,  VERB, NO_SUBTYPE },
    { "were",       4,  VERB, NO_SUBTYPE },
};

static struct known_word *find_known_word(char *word, size_t length);
static char *make_lowercase(char *original, size_t length);

bool
grammar_prohibits(char *prev, size_t prev_length,
                  char *next, size_t next_length,
                  unsigned int options)
{
    bool prohibited = false;
    char *lw, *nw;          /* last and next words */
    char *lw_low, *nw_low;  /* lowercase versions for case insensitivity */
    size_t lw_len, nw_len;  /* lengths of those words */
    struct known_word *kp, *kn; /* matching entries in known_words */

    if ((prev == NULL)
        || (next == NULL)
        || (next_length == 0))
        return true;
    else if (prev_length == 0)
        return false; /* this is fine; we just haven't started a sentence */

    lw_low = NULL;
    nw_low = NULL;

    /* Last word of the previous phrase */
    lw = phrase_last_word(prev, prev_length, &lw_len);
    if ((lw_low = make_lowercase(lw, lw_len)) == NULL)
        goto cleanup;

    /* First word of the next phrase */
    nw = phrase_first_word(next, next_length, &nw_len);
    if ((nw_low = make_lowercase(nw, nw_len)) == NULL)
        goto cleanup;

    /* Are we allowed to repeat the last word? */
    if ((options & GA_NO_REPEATS)
        && (lw_len == nw_len)
        && (strncmp(lw_low, nw_low, lw_len) == 0)) {
        prohibited = true;
        goto cleanup;
    }

    /* Do we know anything about these words? */
    kp = find_known_word(lw_low, lw_len);
    kn = find_known_word(nw_low, nw_len);
    if ((kp == NULL) || (kn == NULL))
        goto cleanup;

    /* Do we see anything obviously wrong with this combination? */
    if (kp->type == kn->type) {
        if (kp->type == ARTICLE)
            prohibited = true;  /* two consecutive articles */
        else if ((kp->type == PRONOUN)
                 && (kp->subtype != AMBIGUOUS)
                 && (kp->subtype == kn->subtype))
            prohibited = true;  /* two pronouns with the same case */
    } else if (kp->type == ARTICLE) {
        if (kn->type == PRONOUN)
            prohibited = true;  /* an article modifying a pronoun */
        else if ((kn->type == PREPOSITION)
                 || (kn->type == CONJUNCTION)
                 || (kn->type == VERB))
        prohibited = true;      /* an article modifying a non-noun */
    }

cleanup:
    if (lw_low != NULL)
        free(lw_low);
    if (nw_low != NULL)
        free(nw_low);
    return prohibited;
}

/*
 * If we know this word, return its entry in our list of known words.
 * Otherwise, return NULL.
 */
struct known_word *
find_known_word(char *word, size_t length)
{
    size_t i;

    /* Do we even know any words of this length? */
    if (length > MAX_KNOWN_WORD_LENGTH)
        return NULL;

    for (i = 0; i < KNOWN_WORD_COUNT; ++i) {
        if ((known_words[i].length == length)
            && strncmp(known_words[i].word, word, length) == 0)
            return &known_words[i];
    }
    return NULL;
}

/*
 * Duplicate a string and make it lowercase.
 */
char *
make_lowercase(char *original, size_t length)
{
    char *buf;
    size_t i;

    buf = malloc((length + 1) * sizeof(char));
    if (buf != NULL) {
        for (i = 0; i < length; ++i)
            buf[i] = tolower(original[i]);
        buf[length] = '\0';
    }
    return buf;
}

#ifdef TEST
#   define ONE_WORD     "Hello"
#   define ONE_LOWER    "hello"     /* the same, but lowercase */
#   define TWO_WORDS    "Hello world"
#   define MANY_WORDS   "Hello darkness my old friend"
#   define PREP_1       "in"
#   define PREP_2       "out"
#   define ART_1        "a"
#   define ART_2        "an"
#   define PREP_ART     "above the" /* preposition and article */
#   define ONE_PRONOUN  "I"
#   define QWANTZLE_1   "in the"    /* a 2-gram from the Qwantzle... */
#   define QWANTZLE_2   "to be"     /* ...that shouldn't precede this one */

grammar_t
grammar_type(char *word, size_t length)
{
    struct known_word *kw;

    if ((kw = find_known_word(word, length)) == NULL)
        return UNCLASSIFIED;
    else
        return kw->type;
}

void
identify_words(struct phrase_list *item)
{
    char *word, *buf;
    size_t length;
    unsigned int ga_options;

    ga_options = GA_NO_REPEATS;

    printf("%-15s \"%s\" (length %zu)\n",
           "Phrase:", item->phrase, item->length);

    word = phrase_first_word(item->phrase, item->length, &length);
    if ((buf = make_lowercase(word, length)) != NULL) {
        printf("%-15s \"%s\" (length %zu, type %u)\n",
               "First word:", buf, length, grammar_type(buf, length));
        free(buf);
    }

    word = phrase_last_word(item->phrase, item->length, &length);
    if ((buf = make_lowercase(word, length)) != NULL) {
        printf("%-15s \"%s\" (length %zu, type %u)\n",
               "Last word:", buf, length, grammar_type(buf, length));
        free(buf);
    }

    if (item->next != NULL)
        printf("%-15s %s\n", "Allow next:",
               grammar_prohibits(item->phrase, item->length,
                                 item->next->phrase, item->next->length,
                                 ga_options) ? "no" : "yes");

    printf("\n");
}

int
main(int argc, char **argv)
{
    struct phrase_list *list, *last, *curr;
    size_t count;

    count = 0;
    list = last = phrase_list_add(NULL, ONE_WORD, strlen(ONE_WORD), &count);
    last = phrase_list_add(last, ONE_LOWER, strlen(ONE_LOWER), &count);
    last = phrase_list_add(last, TWO_WORDS, strlen(TWO_WORDS), &count);
    last = phrase_list_add(last, MANY_WORDS, strlen(MANY_WORDS), &count);
    last = phrase_list_add(last, PREP_1, strlen(PREP_1), &count);
    last = phrase_list_add(last, PREP_ART, strlen(PREP_ART), &count);
    last = phrase_list_add(last, ART_1, strlen(ART_1), &count);
    last = phrase_list_add(last, PREP_2, strlen(PREP_2), &count);
    last = phrase_list_add(last, ART_2, strlen(ART_2), &count);
    last = phrase_list_add(last, ONE_PRONOUN, strlen(ONE_PRONOUN), &count);
    last = phrase_list_add(last, QWANTZLE_1, strlen(QWANTZLE_1), &count);
    last = phrase_list_add(last, QWANTZLE_2, strlen(QWANTZLE_2), &count);

    for (curr = list; curr != NULL; curr = curr->next)
        identify_words(curr);

    phrase_list_free(list);
    return 0;
}
#endif /* TEST */
