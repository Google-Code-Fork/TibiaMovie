/*
 * main.c: where it all begins...
 *
 * Copyright 2004
 * See the COPYING file for more information on licensing and use.
 *
 * This file contains the initial WinMain(), the main message-handling
 * function, as well as some other small functions.
 */

#include <windows.h>
#include <winsock2.h>
#include <stdio.h>
#include <stdarg.h>
#include "tibiamovie.h"

LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void OnPaint(HWND hwnd, int message, WPARAM wParam, LPARAM lParam);

char *szClassName = "TibiaMovie";

/* our windows */
HWND wMain         = NULL;
HWND btnActivate   = NULL;
HWND btnPlay       = NULL;
HWND btnRecord     = NULL;
HWND btnRecordOpen = NULL;
HWND btnAddMarker  = NULL;
HWND btnGoToMarker = NULL;
HWND btnServers    = NULL;

int mode = 0;

int debug = 0;

/* filename saving stuff */
OPENFILENAME ofn;
char *ofntitle = "Record Movie to...";
char *ofndefext = "tmv";

char saveFile[512];     /* path+filename to save to */
char saveFileBase[512]; /* filename to save to */

int WINAPI WinMain(HINSTANCE hThisInstance, HINSTANCE hPrevInstance, char *lpszArgument, int show)
{
    HWND hwnd;
    MSG messages;
    WNDCLASSEX wincl;

    wincl.hInstance     = hThisInstance;
    wincl.lpszClassName = szClassName;
    wincl.lpfnWndProc   = WindowProcedure;
    wincl.style         = CS_DBLCLKS;
    wincl.cbSize        = sizeof(WNDCLASSEX);
    wincl.hIcon         = LoadIcon(hThisInstance, MAKEINTRESOURCE(100));
    wincl.hIconSm       = LoadIcon(hThisInstance, MAKEINTRESOURCE(100));
    wincl.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wincl.lpszMenuName  = NULL;
    wincl.cbClsExtra    = 0;
    wincl.cbWndExtra    = 0;
    wincl.hbrBackground = (HBRUSH)COLOR_BACKGROUND;

    if (!RegisterClassEx(&wincl))
        return 0;

    hwnd = CreateWindowEx(
           0,
           szClassName,
           "TibiaMovie",
           WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
           CW_USEDEFAULT,
           CW_USEDEFAULT,
           250, /* width */
           290, /* height */
           HWND_DESKTOP,
           NULL,
           hThisInstance,
           NULL
           );

    wMain = hwnd;

    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.hInstance = hThisInstance;
    ofn.lpstrCustomFilter = NULL;
    ofn.nFilterIndex = 0;
    ofn.lpstrFilter = TEXT("TibiaMovie Files (*.TMV)\0*.tmv\0\0");
    ofn.lpstrFile = saveFile;
    ofn.lpstrTitle = ofntitle;
    ofn.nMaxFile = 511;
    ofn.lpstrDefExt = ofndefext;
    ofn.Flags =   OFN_PATHMUSTEXIST      | OFN_HIDEREADONLY     | OFN_NOCHANGEDIR
                | OFN_NONETWORKBUTTON    | OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT
                | OFN_EXTENSIONDIFFERENT
              ;

    strcpy(saveFile, "movie0001.tmv");
    strcpy(saveFileBase, "movie0001.tmv");
    
    ShowWindow(hwnd, show);

    while (GetMessage(&messages, NULL, 0, 0)) {
        TranslateMessage(&messages);
        DispatchMessage(&messages);
    }

    return messages.wParam;
}

LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
        case WM_DESTROY: {
            /* we're leaving, so try to put back tibia's memory the way it was */
            if (memoryActivated)
                MemoryInjection(0);

            PostQuitMessage(0);
            break;
        }
        case WM_CREATE: {
            WSADATA WSAData;
            int iError;

            /* create our child windows */
            btnActivate   = CreateWindow("button", "Activate",          WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,   0,   0, 244,  25, hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
            btnPlay       = CreateWindow("button", "Play",              WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,   0,  25, 122,  25, hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
            btnRecord     = CreateWindow("button", "Record",            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 122,  25, 102,  25, hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
            btnRecordOpen = CreateWindow("button", "...",               WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 224,  25,  20,  25, hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
            btnAddMarker  = CreateWindow("button", "Add Marker",        WS_CHILD |              BS_PUSHBUTTON,   0, 240, 244,  25, hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
            btnGoToMarker = CreateWindow("button", "Go To Next Marker", WS_CHILD |              BS_PUSHBUTTON,   0, 240, 244,  25, hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
            btnServers    = CreateWindow("listbox", NULL,               WS_CHILD | (LBS_STANDARD & ~LBS_SORT),   0, 153, 144,  90, hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

            RecordFillServerBox();

            /* select the first server */
            SendMessage(btnServers, LB_SETCURSEL, (WPARAM)0, (LPARAM)0);
            
            /* adjust our privileges so we can read/write tibia's memory :) */
            AdjustPrivileges();

            /* start up winsock2 */
            if ((iError = WSAStartup(MAKEWORD(2,0), &WSAData))) {
                char buf[512];
                sprintf(buf, "Error in WSAStartup(): %d", iError);
                MessageBox(NULL, buf, szClassName, 0);
                PostQuitMessage(0);
            }

            break;
        }
        case WM_COMMAND: {
            /* Activate button pressed */
            if ((HWND)lParam == btnActivate) {
                if (!memoryActivated)
                    MemoryInjection(1);
                else
                    MemoryInjection(0);
                
                if (memoryActivated)
                    SetWindowText(btnActivate, "Deactivate");
                else
                    SetWindowText(btnActivate, "Activate");
                    
            }
            /* Play button pressed */
            else if ((HWND)lParam == btnPlay) {
                if (mode == MODE_NONE) {
                    mode = MODE_PLAY;
                    PlayStart();
                }
                else if (mode == MODE_RECORD || mode == MODE_RECORD_PAUSE)
                    ;
                else {
                    mode = MODE_NONE;
                    PlayEnd();
                }
                    
                if (mode == MODE_PLAY) {
                    ShowWindow(btnGoToMarker, 1);
                    SetWindowText(btnPlay, "Stop");
                }
                else {
                    ShowWindow(btnGoToMarker, 0);
                    SetWindowText(btnPlay, "Play");
                }
            }
            /* Record button pressed */
            else if ((HWND)lParam == btnRecord) {
                if (mode == MODE_NONE) {
                    FindUnusedMovieName();
                    mode = MODE_RECORD;
                    RecordStart();
                }
                else if (mode == MODE_PLAY)
                    ;
                else if (mode == MODE_RECORD) {
                    mode = MODE_RECORD_PAUSE;
                    RecordEnd();
                }
                else if (mode == MODE_RECORD_PAUSE) {
                    mode = MODE_NONE;
                    RecordDisconnect();
                }

                if (mode == MODE_RECORD) {
                    ShowWindow(btnAddMarker, 1);
                    ShowWindow(btnServers, 1);
                    SetWindowText(btnRecord, "Stop");
                }
                else if (mode == MODE_RECORD_PAUSE) {
                    ShowWindow(btnAddMarker, 0);
                    ShowWindow(btnServers, 0);
                    SetWindowText(btnRecord, "Disconnect");
                }
                else {
                    ShowWindow(btnAddMarker, 0);
                    ShowWindow(btnServers, 0);
                    SetWindowText(btnRecord, "Record");
                }
            }
            /* ... button pressed */
            else if ((HWND)lParam == btnRecordOpen) {
                 if (GetSaveFileName(&ofn)) {
                     char *c;
                     
                     c = saveFile + strlen(saveFile);
                     
                     while (c > saveFile) {
                         if (*c == '\\') {
                             strcpy(saveFileBase, c + 1);
                             break;
                         }
                         c--;
                     }
                 }
                 else {
                     saveFile[0] = 0;
                     FindUnusedMovieName();
                 }
            }
            /* Add Marker button pressed */
            else if ((HWND)lParam == btnAddMarker) {
                RecordAddMarker();
            }
            /* Go To Next Marker button pressed */
            else if ((HWND)lParam == btnGoToMarker) {
                fastForwarding = 1;
                playSpeed = 10;
            }

            InvalidateRect(hwnd, NULL, TRUE);
            break;
        }
        case WM_LBUTTONDOWN: case WM_MOUSEMOVE: {
            int fwKeys = wParam;
            int xPos = LOWORD(lParam);
            int yPos = HIWORD(lParam);
            POINT pt;
            RECT rect;
            
            if (message == WM_LBUTTONDOWN || (message == WM_MOUSEMOVE && (fwKeys & MK_LBUTTON))) {
               SetRect(&rect, 60, 91, 225, 104);
               pt.x = xPos;
               pt.y = yPos;
               
               if (PtInRect(&rect, pt)) {
                   int newspeed = ((pt.x - 60) / 15);
                   if (newspeed != playSpeed) {
                       playSpeed = newspeed;
                       InvalidateRect(hwnd, &rect, TRUE);
                   }
               }
            }
            break;
        }
        case WM_PAINT: {
            OnPaint(hwnd, message, wParam, lParam);
            break;
        }
        case WM_SOCKET_PLAY: {
           int wEvent = WSAGETSELECTEVENT(lParam);
           int wError = WSAGETSELECTERROR(lParam);
           int s = (int)wParam;

           DoSocketPlay(hwnd, wEvent, wError, s);
           break;
        }
        case WM_SOCKET_RECORD: {
           int wEvent = WSAGETSELECTEVENT(lParam);
           int wError = WSAGETSELECTERROR(lParam);
           int s = (int)wParam;

           DoSocketRecord(hwnd, wEvent, wError, s);
           break;
        }
        default: {
            return DefWindowProc (hwnd, message, wParam, lParam);
        }
    }

    return 0;
}

void FindUnusedMovieName(void)
{
    char buf[512];
    int cnt;
    FILE *fpCheck = NULL;

    if (saveFile[0] != 0)
        fpCheck = fopen(saveFile, "r");

    if (saveFile[0] != 0 && fpCheck == NULL)
        return;

    else if (fpCheck)
        fclose(fpCheck);

    for (cnt = 1; cnt < 10000; cnt++) {
        sprintf(buf, "movie%04d.tmv", cnt);

        if ((fpCheck = fopen(buf, "r")) != 0) {
            fclose(fpCheck);
        }
        else
            break;
    }

    strcpy(saveFile, buf);
    strcpy(saveFileBase, buf);
    return;
}

void OnPaint(HWND hwnd, int message, WPARAM wParam, LPARAM lParam)
{
    int y;
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rect;
    HBRUSH brushBlack, brushBlue, brushLtBlue;
    RECT rectDraw;
    char buf[512];

    InvalidateRect(hwnd, &rect, TRUE);

    hdc = BeginPaint(hwnd, &ps);
    SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));
    SetBkMode(hdc, TRANSPARENT);

    brushBlack  = CreateSolidBrush(RGB(0, 0, 0));
    brushBlue   = CreateSolidBrush(RGB(0, 0, 128));
    brushLtBlue = CreateSolidBrush(RGB(150, 150, 255));

    SetRect(&rectDraw, 60, 91, 225, 104);
    FillRect(hdc, &rectDraw, brushBlack);
    SetRect(&rectDraw, 61, 92, 224, 103);
    FillRect(hdc, &rectDraw, brushBlue);
    SetRect(&rectDraw, 60 + (playSpeed * 15), 91, 60 + ((playSpeed + 1) * 15), 104);
    FillRect(hdc, &rectDraw, brushBlack);
    SetRect(&rectDraw, 60 + (playSpeed * 15), 92, 60 + ((playSpeed + 1) * 15), 103);
    FillRect(hdc, &rectDraw, brushLtBlue);

    sprintf(buf, "Memory Injection: %s", memoryActivated ? "Enabled" : "Disabled");
    TextOut(hdc, 0, 60, buf, strlen(buf));
    sprintf(buf, "Mode: %s", mode == MODE_NONE ? "None" : mode == MODE_PLAY ? "Play" : "Record");
    TextOut(hdc, 0, 75, buf, strlen(buf));
    TextOut(hdc, 0, 90, "Speed: ", 7);

    y = 105;

    if (mode == MODE_RECORD) {
        if (bytesRecorded > 0) {
            sprintf(buf, "Bytes Recorded: %d", bytesRecorded);
            TextOut(hdc, 0, y, buf, strlen(buf));
            y += 15;
        }

        sprintf(buf, "Record To: %s", saveFileBase);
        TextOut(hdc, 0, y, buf, strlen(buf));
        y += 15;

        if (numMarkers > 0) {
            sprintf(buf, "Markers Set: %d", numMarkers);
            TextOut(hdc, 0, y, buf, strlen(buf));
            y += 15;
        }
    }
    else if (mode == MODE_PLAY) {
        if (bytesPlayed > 0) {
           sprintf(buf, "Bytes Played: %d", bytesPlayed);
           TextOut(hdc, 0, y, buf, strlen(buf));
           y += 15;
        }
        if (msPlayed > 0 && msTotal > 0) {
            char buf2[32];

            sprintf(buf, "Position: %s (%.02f%%)", duration(msPlayed / 1000, buf2), msPlayed * 100.0 / msTotal);
            TextOut(hdc, 0, y, buf, strlen(buf));
            y += 15;
            sprintf(buf, "Length: %s", duration(msTotal / 1000, buf2));
            TextOut(hdc, 0, y, buf, strlen(buf));
            y += 15;
        }
    }

    DeleteObject(brushBlack);
    DeleteObject(brushBlue);
    DeleteObject(brushLtBlue);
    EndPaint(hwnd, &ps);
}

void debugf(char *fmt, ...)
{
    char buf[8192];
    va_list args;
    FILE *fp;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf) - 1, fmt, args);
    va_end(buf);

    fp = fopen("debug.txt", "a");

    if (fp) {
        fprintf(fp, "%s", buf);
        fflush(fp);
        fclose(fp);
    }

    return;
}

