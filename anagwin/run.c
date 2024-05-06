/*
 * The part that does the actual finding of anagrams.
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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "anagwin.h"

static DWORD WINAPI RunAnagramSearchThread(LPVOID lpParam);
static void first_phrase_cb(char *candidate, void *user_data);
static void progress_cb(void *user_data);
static void sentence_cb(char *sentence, void *user_data);
static void sentence_cb_inner(char *sentence, struct anagram_window *window);
static void finished_cb(void *user_data);

/*
 * Start searching for anagrams.
 */
void
StartAnagramSearch(struct anagram_window *window)
{
    FILE *fp;
    pool_t pool[POOL_SIZE];
    char *subject, max_words_text[3];
    size_t max_words;
    int subject_len;
    unsigned short i;

    if (window == NULL)
        return;

    StopAnagramSearch(window);
    ClearAnagramSearchResults(window);

    fp = NULL;
    subject = NULL;
    max_words = 0;
    pool_reset(pool);

    /* Reset the progress bar */
    SendMessage(window->hwndProgressBar, PBM_SETPOS, 0, 0);

    /* Add letters to the pool */
    subject_len = GetWindowTextLength(window->hwndSubject) + 1;
    if (subject_len <= 1) /* counting the null byte */
        goto cleanup;
    subject = malloc(subject_len * sizeof(char));
    if (subject == NULL)
        goto cleanup;
    GetWindowText(window->hwndSubject, subject, subject_len);
    pool_add(pool, subject);
    free(subject);
    subject = NULL;

    /* Limit results to a reasonable number of words */
    GetWindowText(window->hwndLimit, max_words_text, 3);
    max_words = max(MIN_WORDS,
                    min(MAX_WORDS, strtoul(max_words_text, NULL, 0)));

    /* This allows only one thread to add to window->anagrams at a time */
    window->hMutex = CreateMutex(NULL, false, NULL);
    if (window->hMutex == NULL)
        goto cleanup;

    window->si = malloc(num_threads * sizeof(struct sentence_info*));
    if (window->si == NULL)
        goto cleanup;
    memset(window->si, 0, num_threads * sizeof(struct sentence_info*));

    window->hThreadArray = malloc(num_threads * sizeof(HANDLE));
    if (window->hThreadArray == NULL)
        goto cleanup;

    /* Read in our phrase list */
    fp = fopen(window->list_path, "r");
    if (fp == NULL) {
        MessageBox(window->hwnd,
                   "Failed to read phrase list.",
                   "Error",
                   MB_OK | MB_ICONERROR);
        goto cleanup;
    }
    window->phrase_list =
        phrase_list_read(NULL, fp, &window->phrase_count, pool);
    fclose(fp);

    /* Set the progress bar range */
    SendMessage(window->hwndProgressBar,
                PBM_SETRANGE32, 0, window->phrase_count);

    /* Start threads */
    for (i = 0; i < num_threads; ++i) {
        window->si[i] = malloc(sizeof(struct sentence_info));
        if (window->si[i] == NULL)
            goto cleanup;

        sentence_info_init(window->si[i]);
        pool_copy(pool, window->si[i]->pool);
        window->si[i]->phrase_list = window->phrase_list;
        window->si[i]->phrase_count = window->phrase_count;
        window->si[i]->user_data = window;
        window->si[i]->max_words = max_words;
        window->si[i]->offset = i;
        window->si[i]->step = num_threads;
        window->si[i]->first_phrase_cb = first_phrase_cb;
        window->si[i]->progress_cb = progress_cb;
        window->si[i]->sentence_cb = sentence_cb;
        window->si[i]->finished_cb = finished_cb;

        window->hThreadArray[i] = CreateThread(
            NULL,                       /* default security attributes */
            0,                          /* use default stack size */
            RunAnagramSearchThread,     /* thread function name */
            window->si[i],              /* argument to thread function */
            0,                          /* use default creation flags */
            NULL                        /* we don't need the thread ID */
        );

        if (window->hThreadArray[i] == NULL)
            goto cleanup;
        ++window->running_threads;
    }

    EnableWindow(window->hwndCancelButton, true);
    return;

cleanup:
    if (subject != NULL)
        free(subject);
    if (fp != NULL)
        fclose(fp);
    StopAnagramSearch(window);
}

/*
 * Cancel a running anagram search.
 * This does not clear results.
 */
void
StopAnagramSearch(struct anagram_window *window)
{
    unsigned short i;

    if (window == NULL)
        return;

    SendMessage(window->hwndStatusBar,
                SB_SETTEXT, MAKEWPARAM(1, 0), (LPARAM) NULL);
    EnableWindow(window->hwndCancelButton, false);

    if (window->hThreadArray != NULL) {
        for (i = 0; i < num_threads; ++i) {
            TerminateThread(window->hThreadArray[i], 1);
            CloseHandle(window->hThreadArray[i]);
        }

        free(window->hThreadArray);
        window->hThreadArray = NULL;
    }
    window->running_threads = 0;

    if (window->si != NULL) {
        for (i = 0; i < num_threads; ++i)
            free(window->si[i]);

        free(window->si);
        window->si = NULL;
    }

    if (window->hMutex != NULL) {
        CloseHandle(window->hMutex);
        window->hMutex = NULL;
    }

    if (window->phrase_list != NULL) {
        phrase_list_free(window->phrase_list);
        window->phrase_list = NULL;
    }
    window->phrase_count = 0;
}

/*
 * Clear the results of the previous anagram search.
 */
void
ClearAnagramSearchResults(struct anagram_window *window)
{
    if (window == NULL)
        return;

    SendMessage(window->hwndAnagrams, LB_RESETCONTENT, 0, 0);

    if (window->anagrams != NULL) {
        phrase_list_free(window->anagrams);
        window->anagrams = NULL;
    }

    window->last_anagram = NULL; /* this was in window->anagrams */
    window->anagram_count = 0;
}

/*
 * Helper function to run an anagram search thread.
 */
DWORD WINAPI
RunAnagramSearchThread(LPVOID lpParam)
{
    struct sentence_info *si = lpParam;

    sentence_build(si);
    return 1; /* ignored */
}

/*
 * Callback to indicate a change in first phrase.
 */
void
first_phrase_cb(char *candidate, void *user_data)
{
    struct anagram_window *window = user_data;
    char buf[MAX_STATUS];

    if (snprintf(buf, MAX_STATUS,
                 "Finding anagrams starting with %s...", candidate) != 0)
        SendMessage(window->hwndStatusBar,
                    SB_SETTEXT, MAKEWPARAM(1, 0), (LPARAM) buf);
}

/*
 * Callback to update our sentence-building progress.
 */
void
progress_cb(void *user_data)
{
    struct anagram_window *window = user_data;
    SendMessage(window->hwndProgressBar, PBM_STEPIT, 0, 0);
}

/*
 * Callback function when a sentence is completed.
 */
void
sentence_cb(char *sentence, void *user_data)
{
    struct anagram_window *window = user_data;
    DWORD dwResult;

    dwResult = WaitForSingleObject(window->hMutex, INFINITE);
    switch (dwResult) {
        case WAIT_OBJECT_0:
            /* Our turn to add to the list */
            sentence_cb_inner(sentence, window);
            ReleaseMutex(window->hMutex);
            break;

        case WAIT_ABANDONED:
            /* Abandoned mutex (we should never see this) */
            return;
    }
}

void
sentence_cb_inner(char *sentence, struct anagram_window *window)
{
    window->last_anagram = phrase_list_add(window->last_anagram,
                                           sentence,
                                           &window->anagram_count);
    if (window->last_anagram == NULL)
        ExitProcess(1); /* we've run out of memory */
    else if (window->anagrams == NULL)
        window->anagrams = window->last_anagram; /* this is the first one */

    /* Display the result */
    SendMessage(window->hwndAnagrams, LB_ADDSTRING,
                0, (LPARAM) window->last_anagram->phrase);
}

/*
 * Callback when a search thread finishes.
 */
void
finished_cb(void *user_data)
{
    struct anagram_window *window = user_data;
    DWORD dwResult;

    dwResult = WaitForSingleObject(window->hMutex, INFINITE);
    switch (dwResult) {
        case WAIT_OBJECT_0:
            --window->running_threads;
            ReleaseMutex(window->hMutex);
            break;

        case WAIT_ABANDONED:
            return;
    }

    if (window->running_threads == 0) {
        SendMessage(window->hwndStatusBar,
                    SB_SETTEXT, MAKEWPARAM(1, 0), (LPARAM) NULL);
        EnableWindow(window->hwndCancelButton, false);
    }
}
