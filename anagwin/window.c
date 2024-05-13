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
#include <string.h>

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
    char buf[MAX_STATUS]; /* buffer for status bar messages */

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

        case WM_START_SEARCH:
            if (window->running_threads++ == 0)
                EnableWindow(window->hwndCancelButton, true);
            return 0;

        case WM_STOP_SEARCH:
            if (--window->running_threads == 0) {
                char buf[MAX_STATUS];
                if (snprintf(buf, MAX_STATUS, "Found %zu anagrams.",
                             window->anagram_count) != 0)
                    SendMessage(window->hwndStatusBar,
                                SB_SETTEXT, MAKEWPARAM(1, 0), (LPARAM) buf);

                EnableWindow(window->hwndCancelButton, false);
            }
            return 0;

        case WM_CANCEL_SEARCH:
            SendMessage(window->hwndStatusBar,
                        SB_SETTEXT, MAKEWPARAM(1, 0), (LPARAM) NULL);
            EnableWindow(window->hwndCancelButton, false);
            return 0;

        case WM_FIRST_PHRASE:
            if (snprintf(buf, MAX_STATUS,
                         "Finding anagrams starting with %s...",
                         (char*)lParam) != 0)
                SendMessage(window->hwndStatusBar,
                            SB_SETTEXT, MAKEWPARAM(1, 0), (LPARAM) buf);
            return 0;

        case WM_SIZE:
            LayOutAnagramWindow(window);
            /* The status bar resizes itself */
            SendMessage(window->hwndStatusBar, WM_SIZE, wParam, lParam);
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
    int nStatusParts[] = { 2 * BUTTON_WIDTH, -1 };

    window = malloc(sizeof(struct anagram_window));
    if (window == NULL)
        return -1;
    memset(window, 0, sizeof(struct anagram_window));

    window->hwnd = hwnd;
    SetWindowLongPtr(window->hwnd, GWLP_USERDATA, (LONG_PTR)window);

    /* Just create the widgets now and worry about positioning them later */
    window->hwndSubjectLabel = CreateWindowEx(
        /* dwExStyle */     0,
        /* lpClassName */   WC_STATIC,
        /* lpWindowName */  LABEL_SUBJECT,
        /* dwStyle */       WS_CHILD | WS_VISIBLE,
        /* pos, size */     0, 0, 0, 0,
        /* hwndParent */    hwnd,
        /* hMenu */         NULL,
        /* hInstance */     hInstance,
        /* lpParam */       NULL
    );
    window->hwndSubject = CreateWindowEx(
        /* dwExStyle */     WS_EX_CLIENTEDGE,
        /* lpClassName */   WC_EDIT,
        /* lpWindowName */  NULL,
        /* dwStyle */       WS_CHILD | WS_TABSTOP | WS_VISIBLE
                            | ES_LEFT,
        /* pos, size */     0, 0, 0, 0,
        /* hwndParent */    hwnd,
        /* hMenu */         NULL,
        /* hInstance */     hInstance,
        /* lpParam */       NULL
    );
    window->hwndLimitLabel = CreateWindowEx(
        /* dwExStyle */     0,
        /* lpClassName */   WC_STATIC,
        /* lpWindowName */  LABEL_LIMIT,
        /* dwStyle */       WS_CHILD | WS_VISIBLE,
        /* pos, size */     0, 0, 0, 0,
        /* hwndParent */    hwnd,
        /* hMenu */         NULL,
        /* hInstance */     hInstance,
        /* lpParam */       NULL
    );
    window->hwndLimit = CreateWindowEx(
        /* dwExStyle */     WS_EX_CLIENTEDGE,
        /* lpClassName */   WC_EDIT,
        /* lpWindowName */  "2",
        /* dwStyle */       WS_CHILD | WS_TABSTOP | WS_VISIBLE
                            | ES_LEFT | ES_NUMBER,
        /* pos, size */     0, 0, 0, 0,
        /* hwndParent */    hwnd,
        /* hMenu */         NULL,
        /* hInstance */     hInstance,
        /* lpParam */       NULL
    );
    window->hwndLimitLabelAfter = CreateWindowEx(
        /* dwExStyle */     0,
        /* lpClassName */   WC_STATIC,
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
        /* lpClassName */   WC_BUTTON,
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
        /* lpClassName */   WC_BUTTON,
        /* lpWindowName */  LABEL_CANCEL,
        /* dwStyle */       WS_CHILD | WS_DISABLED | WS_TABSTOP | WS_VISIBLE,
        /* pos, size */     0, 0, 0, 0,
        /* hwndParent */    hwnd,
        /* hMenu */         (HMENU)IDCANCEL,
        /* hInstance */     hInstance,
        /* lpParam */       NULL
    );
    window->hwndAnagrams = CreateWindowEx(
        /* dwExStyle */     WS_EX_CLIENTEDGE,
        /* lpClassName */   WC_LISTBOX,
        /* lpWindowName */  NULL,
        /* dwStyle */       WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_VSCROLL
                            | LBS_NODATA | LBS_NOINTEGRALHEIGHT,
        /* pos, size */     0, 0, 0, 0,
        /* hwndParent */    hwnd,
        /* hMenu */         NULL,
        /* hInstance */     hInstance,
        /* lpParam */       NULL
    );
    window->hwndStatusBar = CreateWindowEx(
        /* dwExStyle */     0,
        /* lpClassName */   STATUSCLASSNAME,
        /* lpWindowName */  NULL,
        /* dwStyle */       WS_CHILD | WS_VISIBLE,
        /* pos, size */     0, 0, 0, 0,
        /* hwndParent */    hwnd,
        /* hMenu */         NULL,
        /* hInstance */     hInstance,
        /* lpParam */       NULL
    );
    window->hwndProgressBar = CreateWindowEx(
        /* dwExStyle */     0,
        /* lpClassName */   PROGRESS_CLASS,
        /* lpWindowName */  NULL,
        /* dwStyle */       WS_CHILD | WS_VISIBLE,
        /* pos, size */     0, 0, 0, 0,
        /* hwndParent */    window->hwndStatusBar,
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
        || (window->hwndAnagrams == NULL)
        || (window->hwndStatusBar == NULL)
        || (window->hwndProgressBar == NULL))
        return -1;

    /* Set a comfortable window font */
    SetAnagramWindowFont(window);

    /* Lay out widgets */
    LayOutAnagramWindow(window);

    /* Customize widgets */
    SendMessage(window->hwndLimit,
                EM_SETLIMITTEXT, (WPARAM) 2, 0);
    SendMessage(window->hwndStatusBar,
                SB_SETPARTS, (WPARAM) 2, (LPARAM) nStatusParts);
    SendMessage(window->hwndProgressBar, PBM_SETSTEP, 1, 0);

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
    RECT rect,
         rcControls,
         rcButtons,
         rcLabels,
         rcInputs,
         rcStatusBar,
         rcAnagrams;

    /* Window rectangle */
    GetClientRect(window->hwnd, &rect);

    /* Rectangle for control widgets */
    rcControls.left = rect.left + WIDGET_MARGIN;
    rcControls.right = rect.right - WIDGET_MARGIN;
    rcControls.top = rect.top + WIDGET_MARGIN;
    rcControls.bottom = rcControls.top
                        + 2 * WIDGET_HEIGHT   /* number of rows */
                        + 1 * ROW_SPACING     /* one fewer than above */
                        + WIDGET_MARGIN;

    /* Place the Start and Cancel buttons at the top right */
    CopyRect(&rcButtons, &rcControls);
    rcButtons.left = rcControls.right - BUTTON_WIDTH;

    MoveWindow(window->hwndStartButton,
               rcButtons.left, rcButtons.top,
               BUTTON_WIDTH, WIDGET_HEIGHT, false);

    rcButtons.top += ROW_SPACING + WIDGET_HEIGHT;

    MoveWindow(window->hwndCancelButton,
               rcButtons.left, rcButtons.top,
               BUTTON_WIDTH, WIDGET_HEIGHT, false);

    /* Place input controls at the top left */
    CopyRect(&rcLabels, &rcControls);
    rcLabels.right = rcLabels.left + LABEL_WIDTH;

    CopyRect(&rcInputs, &rcControls);
    rcInputs.left = rcLabels.right + LABEL_SPACING;
    rcInputs.right = rcButtons.left - WIDGET_MARGIN;

    MoveWindow(window->hwndSubjectLabel,
               rcLabels.left, rcLabels.top,
               LABEL_WIDTH, WIDGET_HEIGHT, false);
    MoveWindow(window->hwndSubject,
               rcInputs.left, rcInputs.top,
               rcInputs.right - rcInputs.left, WIDGET_HEIGHT, false);

    rcLabels.top += ROW_SPACING + WIDGET_HEIGHT;
    rcInputs.top += ROW_SPACING + WIDGET_HEIGHT;

    MoveWindow(window->hwndLimitLabel,
               rcLabels.left, rcLabels.top,
               LABEL_WIDTH, WIDGET_HEIGHT, false);
    MoveWindow(window->hwndLimit,
               rcInputs.left, rcInputs.top,
               40, WIDGET_HEIGHT, false);
    MoveWindow(window->hwndLimitLabelAfter,
               rcInputs.left + 40 + LABEL_SPACING, rcLabels.top,
               100, WIDGET_HEIGHT, false);

    /* Put the progress bar inside the status bar */
    SendMessage(window->hwndStatusBar, SB_GETRECT, 0, (LPARAM) &rcStatusBar);
    rcStatusBar.right -= rcStatusBar.left;  /* more useful as width */
    rcStatusBar.bottom -= rcStatusBar.top;  /* more useful as height */
    MoveWindow(window->hwndProgressBar,
               rcStatusBar.left, rcStatusBar.top,
               rcStatusBar.right, rcStatusBar.bottom, false);

    /* Fill the remaining area with the list of found anagrams */
    CopyRect(&rcAnagrams, &rect);
    if (IsZoomed(window->hwnd)) {
        /* Clip the side border so the scrollbar touches the screen edge */
        rcAnagrams.left -= 2;
        rcAnagrams.right += 2;
    }
    rcAnagrams.top += rcControls.bottom;
    rcAnagrams.bottom -= rcStatusBar.bottom;
    MoveWindow(window->hwndAnagrams,
               rcAnagrams.left,
               rcControls.bottom,
               rcAnagrams.right - rcAnagrams.left,
               rcAnagrams.bottom - rcAnagrams.top,
               false);

    /* Redraw the entire window */
    RedrawWindow(window->hwnd, NULL, NULL, RDW_INVALIDATE);
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
