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

#include <stdio.h>
#include <stdlib.h>

#ifdef UNICODE
#  undef UNICODE
#endif
#include <windows.h>

#include "letter_pool.h"
#include "phrase_list.h"
#include "sentence.h"

/* Minimum and maximum limits for words per anagram */
#define MIN_WORDS 1
#define MAX_WORDS 15

/* Window class name */
#define CLASS_NAME "Anagram"

/* Window metrics */
#define WIDGET_MARGIN   11
#define ROW_SPACING     5
#define WIDGET_HEIGHT   23
#define BUTTON_WIDTH    75
#define LABEL_WIDTH     100
#define LABEL_SPACING   5

/* Menu items and keyboard shortcuts */
#define IDM_CLOSE   1

/* Keyboard accelerators */
#define cAccelAnagram 1
ACCEL accelAnagram[] = {
    { FCONTROL | FVIRTKEY, 'W', IDM_CLOSE }
};

/* Widget labels */
#define LABEL_SUBJECT       "Find anagrams of:"
#define LABEL_LIMIT         "Using:"
#define LABEL_LIMIT_AFTER   "word(s) or fewer"

/* Structure to keep track of window elements */
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

unsigned short num_threads;

static LRESULT CALLBACK AnagramWindowProc(HWND hwnd, UINT uMsg,
                                          WPARAM wParam, LPARAM lParam);
static int CreateAnagramWindow(HWND hwnd);
static void DestroyAnagramWindow(struct anagram_window *window);
static void LayoutAnagramWindow(struct anagram_window *window);
static void SetAnagramWindowFont(struct anagram_window *window);
static bool CALLBACK SetFontCallback(HWND hwnd, LPARAM lParam);

static void AnagramStart(struct anagram_window *window);
static void AnagramCancel(struct anagram_window *window);
static void AnagramReset(struct anagram_window *window);
static DWORD WINAPI RunAnagramThread(LPVOID lpParam);

static void sentence_cb(char *sentence, void *user_data);
static void sentence_cb_inner(char *sentence, struct anagram_window *window);
static void finished_cb(void *user_data);

/*
 * Process window messages.
 */
LRESULT CALLBACK
AnagramWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    struct anagram_window *window =
        (uMsg == WM_CREATE) ? NULL :
        (struct anagram_window*) GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (uMsg) {
        case WM_CREATE:
            return CreateAnagramWindow(hwnd);

        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED)
                switch (LOWORD(wParam)) {
                    case IDOK:
                        AnagramStart(window);
                        return 0;
                    case IDCANCEL:
                        AnagramCancel(window);
                        return 0;
                }
            else /* menu selection or keyboard shortcut */
                switch (LOWORD(wParam)) {
                    case IDM_CLOSE:
                        DestroyWindow(hwnd);
                        return 0;
                }
            break;

        case WM_SIZE:
            LayoutAnagramWindow(window);
            return 0;

        case WM_DESTROY:
            DestroyAnagramWindow(window);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

/*
 * Create the window.
 * Returns 0 on success, -1 on failure.
 */
int
CreateAnagramWindow(HWND hwnd)
{
    struct anagram_window *window;
    HINSTANCE hInstance = (HINSTANCE) GetWindowLongPtr(hwnd, GWLP_HINSTANCE);

    window = malloc(sizeof(struct anagram_window));
    if (window == NULL)
        return -1;
    memset(window, 0, sizeof(struct anagram_window));

    window->hwnd = hwnd;
    SetWindowLongPtr(window->hwnd, GWLP_USERDATA, (LONG_PTR)window);

    /* Just create the widgets now and worry about positioning them later */
    window->hwndSubjectLabel = CreateWindowEx(
        /* dwExStyle */     0,
        /* lpClassName */   "Static",
        /* lpWindowName */  LABEL_SUBJECT,
        /* dwStyle */       WS_CHILD | WS_VISIBLE,
        /* coordinates */   0, 0, 0, 0,
        /* hwndParent */    hwnd,
        /* hMenu */         NULL,
        /* hInstance */     hInstance,
        /* lpParam */       NULL
    );
    window->hwndSubject = CreateWindowEx(
        /* dwExStyle */     0,
        /* lpClassName */   "Edit",
        /* lpWindowName */  NULL,
        /* dwStyle */       WS_BORDER | WS_CHILD | WS_TABSTOP | WS_VISIBLE
                            | ES_LEFT,
        /* coordinates */   0, 0, 0, 0,
        /* hwndParent */    hwnd,
        /* hMenu */         NULL,
        /* hInstance */     hInstance,
        /* lpParam */       NULL
    );
    window->hwndLimitLabel = CreateWindowEx(
        /* dwExStyle */     0,
        /* lpClassName */   "Static",
        /* lpWindowName */  LABEL_LIMIT,
        /* dwStyle */       WS_CHILD | WS_VISIBLE,
        /* coordinates */   0, 0, 0, 0,
        /* hwndParent */    hwnd,
        /* hMenu */         NULL,
        /* hInstance */     hInstance,
        /* lpParam */       NULL
    );
    window->hwndLimit = CreateWindowEx(
        /* dwExStyle */     0,
        /* lpClassName */   "Edit",
        /* lpWindowName */  "2",
        /* dwStyle */       WS_BORDER | WS_CHILD | WS_TABSTOP | WS_VISIBLE
                            | ES_LEFT | ES_NUMBER,
        /* coordinates */   0, 0, 0, 0,
        /* hwndParent */    hwnd,
        /* hMenu */         NULL,
        /* hInstance */     hInstance,
        /* lpParam */       NULL
    );
    window->hwndLimitLabelAfter = CreateWindowEx(
        /* dwExStyle */     0,
        /* lpClassName */   "Static",
        /* lpWindowName */  LABEL_LIMIT_AFTER,
        /* dwStyle */       WS_CHILD | WS_VISIBLE,
        /* coordinates */   0, 0, 0, 0,
        /* hwndParent */    hwnd,
        /* hMenu */         NULL,
        /* hInstance */     hInstance,
        /* lpParam */       NULL
    );
    window->hwndStartButton = CreateWindowEx(
        /* dwExStyle */     0,
        /* lpClassName */   "Button",
        /* lpWindowName */  "Start",
        /* dwStyle */       WS_CHILD | WS_TABSTOP | WS_VISIBLE
                            | BS_DEFPUSHBUTTON,
        /* coordinates */   0, 0, 0, 0,
        /* hwndParent */    hwnd,
        /* hMenu */         (HMENU)IDOK,
        /* hInstance */     hInstance,
        /* lpParam */       NULL
    );
    window->hwndCancelButton = CreateWindowEx(
        /* dwExStyle */     0,
        /* lpClassName */   "Button",
        /* lpWindowName */  "Cancel",
        /* dwStyle */       WS_CHILD | WS_DISABLED | WS_TABSTOP | WS_VISIBLE,
        /* coordinates */   0, 0, 0, 0,
        /* hwndParent */    hwnd,
        /* hMenu */         (HMENU)IDCANCEL,
        /* hInstance */     hInstance,
        /* lpParam */       NULL
    );
    window->hwndAnagrams = CreateWindowEx(
        /* dwExStyle */     0,
        /* lpClassName */   "ListBox",
        /* lpWindowName */  NULL,
        /* dwStyle */       WS_BORDER | WS_CHILD | WS_TABSTOP | WS_VISIBLE
                            | WS_VSCROLL | LBS_NODATA | LBS_NOINTEGRALHEIGHT,
        /* coordinates */   0, 0, 0, 0,
        /* hwndParent */    hwnd,
        /* hMenu */         NULL,
        /* hInstance */     hInstance,
        /* lpParam */       NULL
    );

    /* Make sure all our widgets exist */
    if (window->hwndSubjectLabel == NULL)       return -1;
    if (window->hwndSubject == NULL)            return -1;
    if (window->hwndLimitLabel == NULL)         return -1;
    if (window->hwndLimit == NULL)              return -1;
    if (window->hwndLimitLabelAfter == NULL)    return -1;
    if (window->hwndStartButton == NULL)        return -1;
    if (window->hwndCancelButton == NULL)       return -1;
    if (window->hwndAnagrams == NULL)           return -1;

    /* Set a comfortable window font */
    SetAnagramWindowFont(window);

    /* Size and display resizable widgets */
    LayoutAnagramWindow(window);

    /* Customize widgets */
    SendMessage(window->hwndLimit, EM_SETLIMITTEXT, (WPARAM) 2, 0);

    /* TODO: Support non-default phrase lists */
    window->list_path = phrase_list_default();
    if (window->list_path == NULL)
        return -1;

    SetForegroundWindow(window->hwndSubject);
    return 0;
}

/*
 * Destroy the window.
 * Note DestroyWindow() automatically destroys child windows,
 * so we only need to clean up our own data.
 */
void
DestroyAnagramWindow(struct anagram_window *window)
{
    if (window == NULL)
        return;

    AnagramCancel(window);
    AnagramReset(window);
    DeleteObject(window->hFont);

    if (window != NULL)
        free(window);
}

/*
 * Lay out the window.
 * Called when the window is created or resized.
 */
void
LayoutAnagramWindow(struct anagram_window *window)
{
    RECT rect, rectControls, rectButtons, rectLabels, rectInputs;

    /* Window rectangle */
    GetClientRect(window->hwnd, &rect);

    /* Rectangle for control widgets */
    rectControls.left = rect.left + WIDGET_MARGIN;
    rectControls.right = rect.right - WIDGET_MARGIN;
    rectControls.top = rect.top + WIDGET_MARGIN;
    rectControls.bottom = rectControls.top
                          + 2 * WIDGET_HEIGHT   /* number of rows */
                          + 1 * ROW_SPACING     /* one fewer than above */
                          + WIDGET_MARGIN;

    /* Place the Start and Cancel buttons at the top right */
    CopyRect(&rectButtons, &rectControls);
    rectButtons.left = rectControls.right - BUTTON_WIDTH;

    MoveWindow(window->hwndStartButton,
               rectButtons.left, rectButtons.top,
               BUTTON_WIDTH, WIDGET_HEIGHT, true);

    rectButtons.top += ROW_SPACING + WIDGET_HEIGHT;

    MoveWindow(window->hwndCancelButton,
               rectButtons.left, rectButtons.top,
               BUTTON_WIDTH, WIDGET_HEIGHT, true);

    /* Place input controls at the top left */
    CopyRect(&rectLabels, &rectControls);
    rectLabels.right = rectLabels.left + LABEL_WIDTH;

    CopyRect(&rectInputs, &rectControls);
    rectInputs.left = rectLabels.right + LABEL_SPACING;
    rectInputs.right = rectButtons.left - WIDGET_MARGIN;

    MoveWindow(window->hwndSubjectLabel,
               rectLabels.left, rectLabels.top,
               LABEL_WIDTH, WIDGET_HEIGHT, true);
    MoveWindow(window->hwndSubject,
               rectInputs.left, rectInputs.top,
               rectInputs.right - rectInputs.left, WIDGET_HEIGHT, true);

    rectLabels.top += ROW_SPACING + WIDGET_HEIGHT;
    rectInputs.top += ROW_SPACING + WIDGET_HEIGHT;

    MoveWindow(window->hwndLimitLabel,
               rectLabels.left, rectLabels.top,
               LABEL_WIDTH, WIDGET_HEIGHT, true);
    MoveWindow(window->hwndLimit,
               rectInputs.left, rectInputs.top,
               40, WIDGET_HEIGHT, true);
    MoveWindow(window->hwndLimitLabelAfter,
               rectInputs.left + 40 + LABEL_SPACING, rectLabels.top,
               100, WIDGET_HEIGHT, true);

    /* Fill the remaining area with the list of found anagrams */
    MoveWindow(window->hwndAnagrams,
               rect.left,
               rectControls.bottom,
               rect.right - rect.left,
               rect.bottom - rectControls.bottom,
               true);
}

/*
 * Set the window font.
 * (You'd think there would be an easier way to do this.)
 */
void
SetAnagramWindowFont(struct anagram_window *window)
{
    NONCLIENTMETRICS ncm;

    memset(&ncm, 0, sizeof(NONCLIENTMETRICS));
    ncm.cbSize = sizeof(NONCLIENTMETRICS);
    if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS,
                             sizeof(NONCLIENTMETRICS), &ncm, 0)) {
        HFONT hPrevFont = window->hFont;
        window->hFont = CreateFontIndirect(&ncm.lfMessageFont);

        EnumChildWindows(window->hwnd,
                         (WNDENUMPROC) SetFontCallback,
                         (LPARAM) window->hFont);

        if (hPrevFont != NULL)
            DeleteObject(hPrevFont);
    }
}

bool CALLBACK
SetFontCallback(HWND hwnd, LPARAM lParam)
{
    SendMessage(hwnd, WM_SETFONT, lParam, true);
    return true;
}

/*
 * Start searching for anagrams.
 */
void
AnagramStart(struct anagram_window *window)
{
    FILE *fp;
    pool_t pool[POOL_SIZE];
    char *subject, max_words_text[3];
    size_t max_words;
    int subject_len;
    unsigned short i;

    AnagramCancel(window);
    AnagramReset(window);

    fp = NULL;
    subject = NULL;
    max_words = 0;
    pool_reset(pool);

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
        window->si[i]->sentence_cb = sentence_cb;
        window->si[i]->finished_cb = finished_cb;

        window->hThreadArray[i] = CreateThread(
            NULL,                       /* default security attributes */
            0,                          /* use default stack size */
            RunAnagramThread,           /* thread function name */
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
    AnagramCancel(window);
}

/*
 * Cancel a running anagram search.
 * This does not clear results.
 */
void
AnagramCancel(struct anagram_window *window)
{
    unsigned short i;

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
AnagramReset(struct anagram_window *window)
{
    SendMessage(window->hwndAnagrams, LB_RESETCONTENT, 0, 0);

    if (window->anagrams != NULL) {
        phrase_list_free(window->anagrams);
        window->anagrams = NULL;
    }

    window->last_anagram = NULL; /* this was in window->anagrams */
    window->anagram_count = 0;
}

/*
 * Thread function to search for anagrams.
 */
DWORD WINAPI
RunAnagramThread(LPVOID lpParam)
{
    struct sentence_info *si = lpParam;

    sentence_build(si);
    return 1;
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

    if (window->running_threads == 0)
        EnableWindow(window->hwndCancelButton, false);
}

int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
        PSTR lpCmdLine, int nCmdShow)
{
    HACCEL hAccTable;
    WNDCLASS wc = { };
    MSG msg = { };
    HWND hwndAnagram;

    /* One thread per core */
    if ((num_threads = strtoul(getenv("NUMBER_OF_PROCESSORS"), NULL, 0)) == 0)
        num_threads = 1;

    /* Create the accelerator table */
    hAccTable = CreateAcceleratorTable(accelAnagram, cAccelAnagram);

    /* Register the window class */
    wc.lpfnWndProc = AnagramWindowProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    /* Create the window */
    hwndAnagram = CreateWindowEx(
        /* dwExStyle */     0,
        /* lpClassName */   CLASS_NAME,
        /* lpWindowName */  "Anagram Finder",
        /* dwStyle */       WS_OVERLAPPEDWINDOW,
        /* X */             CW_USEDEFAULT,
        /* Y */             CW_USEDEFAULT,
        /* nWidth */        CW_USEDEFAULT,
        /* nHeight */       CW_USEDEFAULT,
        /* hwndParent */    NULL,
        /* hMenu */         NULL,
        /* hInstance */     hInstance,
        /* lpParam */       NULL
    );

    if (hwndAnagram == NULL)
        return 1;

    /* Show the window */
    ShowWindow(hwndAnagram, nCmdShow);
    SetForegroundWindow(hwndAnagram);

    /* Run the message loop */
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        /* IsDialogMessage() makes tabbing between controls work, and must
         * go after TranslateAccelerator() to not capture its messages. */
        if (!(TranslateAccelerator(hwndAnagram, hAccTable, &msg)
              || IsDialogMessage(hwndAnagram, &msg))) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    /* Clean up and exit */
    DestroyAcceleratorTable(hAccTable);
    return 0;
}
