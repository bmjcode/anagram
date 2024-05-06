/*
 * The anagram finder's user interface.
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

/* Widget labels */
#define LABEL_SUBJECT       "Find &anagrams of:"
#define LABEL_LIMIT         "&Using:"
#define LABEL_LIMIT_AFTER   "word(s) or fewer"
#define LABEL_START         "Start"     /* leave accelerator implied */
#define LABEL_CANCEL        "Cancel"    /* leave accelerator implied */

/* Menu items and keyboard shortcuts */
#define IDM_CLOSE           101
#define IDM_FOCUS_SUBJECT   102
#define IDM_FOCUS_LIMIT     103

/* Keyboard accelerators */
/* Windows provides IDOK (Return pressed) and IDCANCEL (Esc pressed) */
ACCEL accel[] = {
    { FVIRTKEY | FCONTROL,  'W',    IDM_CLOSE           },
    { FVIRTKEY | FALT,      'S',    IDOK                },
    { FVIRTKEY | FALT,      'C',    IDCANCEL            },
    { FVIRTKEY | FALT,      'A',    IDM_FOCUS_SUBJECT   },
    { FVIRTKEY | FALT,      'U',    IDM_FOCUS_LIMIT     },
};
int cAccel = 5;

static LRESULT CALLBACK AnagramWindowProc(HWND hwnd, UINT uMsg,
                                          WPARAM wParam, LPARAM lParam);
static int CreateAnagramWindow(HWND hwnd);
static void DestroyAnagramWindow(struct anagram_window *window);
static void LayOutAnagramWindow(struct anagram_window *window);
static void SetAnagramWindowFont(struct anagram_window *window);
static bool CALLBACK SetFontCallback(HWND hwnd, LPARAM lParam);

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
            switch (LOWORD(wParam)) {
                case IDOK:
                    StartAnagramSearch(window);
                    break;

                case IDCANCEL:
                    StopAnagramSearch(window);
                    break;

                case IDM_CLOSE:
                    DestroyWindow(hwnd);
                    break;

                case IDM_FOCUS_SUBJECT:
                    SetFocus(window->hwndSubject);
                    /* select all text */
                    SendMessage(window->hwndSubject, EM_SETSEL, 0, -1);
                    break;

                case IDM_FOCUS_LIMIT:
                    SetFocus(window->hwndLimit);
                    /* select all text */
                    SendMessage(window->hwndLimit, EM_SETSEL, 0, -1);
                    break;
            }
            return 0;

        case WM_SIZE:
            LayOutAnagramWindow(window);
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
        /* pos, size */     0, 0, 0, 0,
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
        /* pos, size */     0, 0, 0, 0,
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
        /* pos, size */     0, 0, 0, 0,
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
        /* pos, size */     0, 0, 0, 0,
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
        /* pos, size */     0, 0, 0, 0,
        /* hwndParent */    hwnd,
        /* hMenu */         NULL,
        /* hInstance */     hInstance,
        /* lpParam */       NULL
    );
    window->hwndStartButton = CreateWindowEx(
        /* dwExStyle */     0,
        /* lpClassName */   "Button",
        /* lpWindowName */  LABEL_START,
        /* dwStyle */       WS_CHILD | WS_TABSTOP | WS_VISIBLE
                            | BS_DEFPUSHBUTTON,
        /* pos, size */     0, 0, 0, 0,
        /* hwndParent */    hwnd,
        /* hMenu */         (HMENU)IDOK,
        /* hInstance */     hInstance,
        /* lpParam */       NULL
    );
    window->hwndCancelButton = CreateWindowEx(
        /* dwExStyle */     0,
        /* lpClassName */   "Button",
        /* lpWindowName */  LABEL_CANCEL,
        /* dwStyle */       WS_CHILD | WS_DISABLED | WS_TABSTOP | WS_VISIBLE,
        /* pos, size */     0, 0, 0, 0,
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
        /* pos, size */     0, 0, 0, 0,
        /* hwndParent */    hwnd,
        /* hMenu */         NULL,
        /* hInstance */     hInstance,
        /* lpParam */       NULL
    );

    /* Make sure all our widgets exist */
    if ((window->hwndSubjectLabel == NULL)
        || (window->hwndSubject == NULL)
        || (window->hwndLimitLabel == NULL)
        || (window->hwndLimit == NULL)
        || (window->hwndLimitLabelAfter == NULL)
        || (window->hwndStartButton == NULL)
        || (window->hwndCancelButton == NULL)
        || (window->hwndAnagrams == NULL))
        return -1;

    /* Set a comfortable window font */
    SetAnagramWindowFont(window);

    /* Lay out widgets */
    LayOutAnagramWindow(window);

    /* Customize widgets */
    SendMessage(window->hwndLimit, EM_SETLIMITTEXT, (WPARAM) 2, 0);

    /* TODO: Support non-default phrase lists */
    window->list_path = phrase_list_default();
    if (window->list_path == NULL)
        return -1;

    SetFocus(window->hwndSubject);
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

    StopAnagramSearch(window);
    ClearAnagramSearchResults(window);

    if (window->hFont != NULL)
        DeleteObject(window->hFont);
    free(window);
}

/*
 * Lay out the window.
 * Called when the window is created or resized.
 */
void
LayOutAnagramWindow(struct anagram_window *window)
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
 * Register the application's window classes.
 */
void
RegisterAnagramWindowClasses(HINSTANCE hInstance)
{
    WNDCLASS wc = { };

    wc.lpfnWndProc = AnagramWindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = ANAGWIN_MAIN_CLASS;
    RegisterClass(&wc);
}

/*
 * Create the main window menu.
 */
HMENU
CreateAnagramWindowMenu(void)
{
    HMENU hMenu;
    HMENU hMenuFile;

    hMenu = CreateMenu();
    if (hMenu == NULL)
        goto cleanup;

    /* Create the File menu */
    hMenuFile = CreatePopupMenu();
    if (hMenuFile == NULL)
        goto cleanup;
    InsertMenu(hMenuFile, 0, 0, IDM_CLOSE, "E&xit");

    /* Add menus to the menu bar */
    InsertMenu(hMenu, 0, MF_POPUP, (UINT_PTR) hMenuFile, "&File");

    return hMenu;

cleanup:
    if (hMenu != NULL)
        DestroyMenu(hMenu);
    return NULL;
}
