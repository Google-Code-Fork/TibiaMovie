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
#include "zlib.h"
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

HBRUSH brushBlack, brushBlue, brushLtBlue, brushLtGrey, brushGrey, brushYellow, brushWhite;

int mode = 0;

int debug = 0;

/* filename saving stuff */
OPENFILENAME ofn;
char *ofntitle = "Record Movie to...";
char *ofndefext = "tmv";

char saveFile[512];     /* path+filename to save to */
char saveFileBase[512]; /* filename to save to */
HINSTANCE hInstance = NULL;

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
    hInstance = hThisInstance;
    
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
            DeleteObject(brushBlack);
            DeleteObject(brushLtGrey);
            DeleteObject(brushGrey);
            DeleteObject(brushBlue);
            DeleteObject(brushLtBlue);
            DeleteObject(brushYellow);
            DeleteObject(brushWhite);
            
            /* we're leaving, so try to put back tibia's memory the way it was */
            if (memoryActivated)
                MemoryInjection(0);

            PostQuitMessage(0);
            break;
        }
        case WM_CREATE: {
            WSADATA WSAData;
            int iError;

            brushBlack  = CreateSolidBrush(RGB(0, 0, 0));
            brushLtGrey = CreateSolidBrush(RGB(192, 192, 192));
            brushGrey   = CreateSolidBrush(RGB(128, 128, 128));
            brushBlue   = CreateSolidBrush(RGB(0, 0, 128));
            brushLtBlue = CreateSolidBrush(RGB(150, 150, 255));
            brushYellow = CreateSolidBrush(RGB(255, 255, 150));
            brushWhite = CreateSolidBrush(RGB(255, 255, 255));

            /* create our child windows */
            btnActivate   = CreateWindow("button", "Activate",          WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,   0,   0, 244,  25, hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
            btnPlay       = CreateWindow("button", "Play",              WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,   0,  25, 122,  25, hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
            btnRecord     = CreateWindow("button", "Record",            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 122,  25, 102,  25, hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
            btnRecordOpen = CreateWindow("button", "...",               WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 224,  25,  20,  25, hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
            btnAddMarker  = CreateWindow("button", "Add Marker",        WS_CHILD |              BS_PUSHBUTTON,   0, 240, 244,  25, hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
            btnGoToMarker = CreateWindow("button", "Go To Next Marker", WS_CHILD |              BS_PUSHBUTTON,   0, 240, 244,  25, hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
            btnServers    = CreateWindow("listbox", NULL,               WS_CHILD | (LBS_STANDARD & ~LBS_SORT),   0, 135, 244,  110, hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

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
            else if ((HWND)lParam == btnServers) {
                int msg = HIWORD(wParam);
                int index;
                if (msg == LBN_SELCHANGE) {
                     index = SendMessage(btnServers, LB_GETCURSEL, 0, 0);
                     
                     if (index == loginserver_last)
                         DialogBox(hInstance, MAKEINTRESOURCE(200), hwnd, (DLGPROC)RecordChooseServerProc);
                }
                
                break;
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
            
            /* only change speed when in play mode */
            if (mode != MODE_PLAY)
                break;
                
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
    RECT rectDraw;
    char buf[512];

    InvalidateRect(hwnd, &rect, TRUE);

    hdc = BeginPaint(hwnd, &ps);
    SelectObject(hdc, GetStockObject(ANSI_FIXED_FONT));
    SetBkMode(hdc, TRANSPARENT);

    y = 60;
    sprintf(buf, "Memory Injection: %s", memoryActivated ? "Enabled" : "Disabled");
    TextOut(hdc, 0, y, buf, strlen(buf)); y += 15;
    sprintf(buf, "Mode: %s", mode == MODE_NONE ? "None" : mode == MODE_PLAY ? "Play" : "Record");
    TextOut(hdc, 0, y, buf, strlen(buf)); y += 15;

    if (mode == MODE_PLAY) {
        int cnt;
        /* the speed bar */
        SetRect(&rectDraw, 59, y + 1, 226, 104);
        FillRect(hdc, &rectDraw, brushBlack);
        SetRect(&rectDraw, 60, y + 2, 225, 103);
        FillRect(hdc, &rectDraw, brushBlue);
/*        SetRect(&rectDraw, 60, y + 1, 60 + 15, y + 13);
        FillRect(hdc, &rectDraw, brushBlack);
*/        
        for (cnt = 1; cnt <= 10; cnt++) {
            SetRect(&rectDraw, 60 + (cnt * 15), y + 2, 61 + (cnt * 15), y + 13);
            FillRect(hdc, &rectDraw, brushLtBlue);
        }
        
        SetRect(&rectDraw, 60 + (playSpeed * 15), y + 1, 60 + ((playSpeed + 1) * 15), 104);
        FillRect(hdc, &rectDraw, brushBlack);
        SetRect(&rectDraw, 60 + (playSpeed * 15), y + 2, 60 + ((playSpeed + 1) * 15), 103);
        if (playSpeed > 0 && playSpeed < 10)
            FillRect(hdc, &rectDraw, brushLtBlue);
        else if (playSpeed == 10)
            FillRect(hdc, &rectDraw, brushYellow);
        else
            FillRect(hdc, &rectDraw, brushGrey);
            
        TextOut(hdc, 0, y, "Speed: ", 7); y += 15;
    }

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
            int cnt;
            
            sprintf(buf, "Position: %s (%.02f%%)", duration(msPlayed / 1000, buf2), msPlayed * 100.0 / msTotal);
            TextOut(hdc, 0, y, buf, strlen(buf));
            y += 15;
            sprintf(buf, "Length: %s", duration(msTotal / 1000, buf2));
            TextOut(hdc, 0, y, buf, strlen(buf));
            y += 15 * 5;
            SetRect(&rectDraw, 19, y, 225, y + 20);
            FillRect(hdc, &rectDraw, brushBlack);
            SetRect(&rectDraw, 20, y + 1, 224, y + 19);
            FillRect(hdc, &rectDraw, brushGrey);
            SetRect(&rectDraw, 20, y + 1, 20 + ((int)((float)msPlayed / (float)msTotal * 204.0)), y + 19);
            FillRect(hdc, &rectDraw, brushLtGrey);
            
            for (cnt = 0; cnt < playMarkersCnt; cnt++) {
                SetRect(&rectDraw, 20 + ((int)((float)playMarkers[cnt] / (float)msTotal * 204.0)), y + 1, 21 + ((int)((float)playMarkers[cnt] / (int)msTotal * 204.0)), y + 19);
                FillRect(hdc, &rectDraw, brushYellow);
            }
        }
    }

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

