/*
 * An anagram finder for Windows.
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

#ifndef ANAGWIN_H
#define ANAGWIN_H

#ifdef UNICODE
#  undef UNICODE
#endif
#define WINVER 0x400
#include <windows.h>
#include <commctrl.h>

#include "letter_pool.h"
#include "phrase_list.h"
#include "sentence.h"

/* Window class names */
#define ANAGWIN_MAIN_CLASS  "AnagWin Main"

/* Minimum and maximum limits for words per anagram */
#define MIN_WORDS 1
#define MAX_WORDS 15

/* Structure to hold main window elements */
struct anagram_window;

/* The number of sentence_build() threads to run at once */
extern unsigned short num_threads;

/* Implemented in run.c */
void StartAnagramSearch(struct anagram_window *window);
void StopAnagramSearch(struct anagram_window *window);
void ClearAnagramSearchResults(struct anagram_window *window);

/* Implemented in window.c */
void RegisterAnagramWindowClasses(HINSTANCE hInstance);
HMENU CreateAnagramWindowMenu(void);

/* Window metrics */
#define WIDGET_MARGIN   11
#define ROW_SPACING     5
#define WIDGET_HEIGHT   23
#define BUTTON_WIDTH    75
#define LABEL_WIDTH     100
#define LABEL_SPACING   5

/* Maximum length of a status bar message */
#define MAX_STATUS 128

/* Main window elements */
/* This structure is defined here rather than in window.c because it's
 * more efficient for the functions in run.c to manipulate window elements
 * directly than to rely on sending messages to the main HWND. If this were
 * intended for outside use we probably would want to make it opaque for
 * encapsulation, but since this is all internal application code I think
 * we can go on the honor system here. */
struct anagram_window {
    HWND hwnd;
    HWND hwndSubjectLabel;
    HWND hwndSubject;
    HWND hwndLimitLabel;
    HWND hwndLimit;
    HWND hwndLimitLabelAfter;
    HWND hwndStartButton;
    HWND hwndCancelButton;
    HWND hwndAnagrams;
    HWND hwndStatusBar;
    HWND hwndProgressBar;
    HFONT hFont;

    /* Phrase list data */
    const char *list_path;
    struct phrase_list *phrase_list;
    size_t phrase_count;

    /* Storage for hwndAnagrams */
    struct phrase_list *anagrams;
    struct phrase_list *last_anagram;
    size_t anagram_count;
    HANDLE hMutex;

    /* Thread data */
    struct sentence_info **si;
    HANDLE *hThreadArray;
    short running_threads;
};

/* Keyboard accelerators */
extern ACCEL accel[];
extern int cAccel;

#endif /* ANAGWIN_H */
