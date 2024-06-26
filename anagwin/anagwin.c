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

#include <string.h>

#include "anagwin.h"

unsigned short num_threads;

int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
        LPSTR lpCmdLine, int nCmdShow)
{
    int retval = 0;
    HACCEL hAccTable;
    HMENU hMenu;
    HWND hwndAnagram;
    MSG msg = { };
    INITCOMMONCONTROLSEX iccs;

    hAccTable = NULL;
    hMenu = NULL;

    /* Initialize common controls */
    memset(&iccs, 0, sizeof(INITCOMMONCONTROLSEX));
    iccs.dwSize = sizeof(INITCOMMONCONTROLSEX);
    iccs.dwICC = ICC_WIN95_CLASSES;
    if (!InitCommonControlsEx(&iccs)) {
        retval = 1;
        goto cleanup;
    }

    /* One thread per core */
    if ((num_threads = strtoul(getenv("NUMBER_OF_PROCESSORS"), NULL, 0)) == 0)
        num_threads = 1;

    /* Register window classes */
    RegisterAnagramWindowClasses(hInstance);

    /* Create the accelerator table */
    hAccTable = CreateAcceleratorTable(accel, cAccel);
    if (hAccTable == NULL) {
        retval = 1;
        goto cleanup;
    }

    /* Create the main window menu */
    hMenu = CreateAnagramWindowMenu();
    if (hMenu == NULL) {
        retval = 1;
        goto cleanup;
    }

    /* Create the main window */
    hwndAnagram = CreateWindowEx(
        /* dwExStyle */     0,
        /* lpClassName */   ANAGWIN_MAIN_CLASS,
        /* lpWindowName */  "Anagram Finder",
        /* dwStyle */       WS_OVERLAPPEDWINDOW,
        /* X */             CW_USEDEFAULT,
        /* Y */             CW_USEDEFAULT,
        /* nWidth */        CW_USEDEFAULT,
        /* nHeight */       CW_USEDEFAULT,
        /* hwndParent */    NULL,
        /* hMenu */         hMenu,
        /* hInstance */     hInstance,
        /* lpParam */       NULL
    );

    if (hwndAnagram == NULL) {
        retval = 1;
        goto cleanup;
    }

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

cleanup:
    /* Clean up and exit */
    DestroyAcceleratorTable(hAccTable);
    return retval;
}
