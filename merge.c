/*
 * merge.c: merging multiple movies into one.
 *
 * Copyright 2005
 * See the COPYING file for more information on licensing and use.
 *
 * This file contains the resource script reference to our icon.
 */

#include <windows.h>
#include <stdio.h>
#include "zlib.h"
#include "tibiamovie.h"

int num_movies = 0;

int DoMerge(HWND hDlg);

BOOL CALLBACK MergeDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
     switch (message)
     {
     case WM_INITDIALOG:
          return 1;

     case WM_COMMAND:
          switch (LOWORD(wParam)) {
          case IDOK: {
               if (DoMerge(hDlg)) {
                   EndDialog (hDlg, 0);
                   return 1;
               }
               else
                   return 0;
          }
          case IDCANCEL:
               EndDialog (hDlg, 0);
               return 1;
          case MERGE_UP: {
             int cnt = SendMessage(GetDlgItem(hDlg, MERGE_LIST), LB_GETCURSEL, 0, 0);
             char buf[512];
             
             if (num_movies == 0 || cnt == 0)
                 break;
                 
             SendMessage(GetDlgItem(hDlg, MERGE_LIST), LB_GETTEXT, cnt, (LPARAM)buf);
//             MessageBox(NULL, buf, "!", MB_OK);
             
             SendMessage(GetDlgItem(hDlg, MERGE_LIST), LB_DELETESTRING, cnt, 0);
             SendMessage(GetDlgItem(hDlg, MERGE_LIST), LB_INSERTSTRING, cnt - 1, (LPARAM)buf);
             SendMessage(GetDlgItem(hDlg, MERGE_LIST), LB_SETCURSEL, cnt - 1, 0);
             break;
          }
          case MERGE_DOWN: {
             int cnt = SendMessage(GetDlgItem(hDlg, MERGE_LIST), LB_GETCURSEL, 0, 0);
             char buf[512];
             
             if (num_movies == 0 || cnt >= num_movies - 1)
                 break;
                 
             SendMessage(GetDlgItem(hDlg, MERGE_LIST), LB_GETTEXT, cnt, (LPARAM)buf);
//             MessageBox(NULL, buf, "!", MB_OK);
             
             SendMessage(GetDlgItem(hDlg, MERGE_LIST), LB_DELETESTRING, cnt, 0);
             SendMessage(GetDlgItem(hDlg, MERGE_LIST), LB_INSERTSTRING, cnt + 1, (LPARAM)buf);
             SendMessage(GetDlgItem(hDlg, MERGE_LIST), LB_SETCURSEL, cnt + 1, 0);
             break;
          }
          case MERGE_DEL: {
             int cnt = SendMessage(GetDlgItem(hDlg, MERGE_LIST), LB_GETCURSEL, 0, 0);
             
             if (num_movies == 0)
                 break;
                 
             SendMessage(GetDlgItem(hDlg, MERGE_LIST), LB_DELETESTRING, cnt, 0);
             
             if (cnt >= num_movies - 1)
                 SendMessage(GetDlgItem(hDlg, MERGE_LIST), LB_SETCURSEL, cnt - 1, 0);
             else
                 SendMessage(GetDlgItem(hDlg, MERGE_LIST), LB_SETCURSEL, cnt, 0);
                 
             num_movies--;
             break;
          }
          case MERGE_ADD: {
               OPENFILENAME ofn;
               char mergeFile[512];
               char mergeFileBase[512];
               char *mergeTitle = "Select a movie to merge...";
               char *mergeExt = "tmv";
               
                memset(&ofn, 0, sizeof(ofn));
                memset(mergeFile, 0, sizeof(mergeFile));
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hDlg;
                ofn.hInstance = hInstance;
                ofn.lpstrCustomFilter = NULL;
                ofn.nFilterIndex = 0;
                ofn.lpstrFilter = TEXT("TibiaMovie Files (*.TMV)\0*.tmv\0\0");
                ofn.lpstrFile = mergeFile;
                ofn.lpstrTitle = mergeTitle;
                ofn.nMaxFile = 511;
                ofn.lpstrDefExt = mergeExt;
                ofn.Flags =   OFN_PATHMUSTEXIST      | OFN_HIDEREADONLY     | OFN_NOCHANGEDIR
                            | OFN_NONETWORKBUTTON    | OFN_NOREADONLYRETURN /*| OFN_OVERWRITEPROMPT*/
                            | OFN_EXTENSIONDIFFERENT
                          ;
          
                if (GetOpenFileName(&ofn)) {
                     char *c;
                     
                     c = mergeFile + strlen(mergeFile);
                     
                     while (c > mergeFile) {
                         if (*c == '\\') {
                             strcpy(mergeFileBase, c + 1);
                             break;
                         }
                         c--;
                     }
                     
                     SendMessage(GetDlgItem(hDlg, MERGE_LIST), LB_ADDSTRING, 0, (LPARAM)mergeFileBase);
                     SendMessage(GetDlgItem(hDlg, MERGE_LIST), LB_SETCURSEL, num_movies, 0);
                     num_movies++;
                 }
                 else {
                     char buf[512];
                     sprintf(buf, "GetOpenFileName(): Error %d", (int)CommDlgExtendedError());
                     MessageBox(NULL, buf, "!", MB_OK);
                 }
                 break;
              }
          }
          break;
     }
     return 0;
}

int DoMerge(HWND hDlg)
{
    FILE *fpRecord = NULL;
    gzFile fpgzRecord;
    gzFile fpgzIn;
    short version = 0;
    short tibiaversion = 0;
    short len = 0;
    int ret;
    
    long int msTotal = 0, msCombinedTotal = 0, delay = 0;
    unsigned char buf[65500];
    int chunk;
    char nullbuf[2] = {0, 0};
    char gzsaveFile[512];
    
    char outfile[512];
    char infile[512];
    int cnt;
    HWND wList = GetDlgItem(hDlg, MERGE_LIST);
    
    if (!wList) {
        return 0;
    }
    
    if (num_movies <= 1) {
        MessageBox(NULL, "Need to merge at least two movies.", szClassName, MB_OK);
        return 0;
    }
    
    GetDlgItemText(hDlg, MERGE_TO, outfile, 511);
    
    if (outfile[0] == '\0') {
        MessageBox(NULL, "No output filename entered.", szClassName, MB_OK);
        return 0;
    }
    
    fpRecord = fopen(outfile, "wb");
    
    if (!fpRecord) {
        sprintf(buf, "Unable to open '%s', error: %d", outfile, errno);
        MessageBox(NULL, buf, szClassName, MB_OK);
        return 0;
    }
    
    fwrite(&version, 2, 1, fpRecord);
    fwrite(&tibiaversion, 2, 1, fpRecord);
    fwrite(&msTotal, 4, 1, fpRecord);

    for (cnt = 0; cnt < num_movies; cnt++) {
        SendMessage(wList, LB_GETTEXT, cnt, (LPARAM)infile);
        
        fpgzIn = gzopen(infile, "rb");
        
        if (!fpgzIn) {
            sprintf(buf, "Unable to open '%s', error: %d. Aborting merge.", infile, errno);
            MessageBox(NULL, buf, szClassName, MB_OK);
            fclose(fpRecord);
            remove(outfile);
            return 0;
        }
        
        /* set the TibiaMovie revision */
        buf[0] = gzgetc(fpgzIn);
        buf[1] = gzgetc(fpgzIn);
        memcpy(&version, &buf[0], 2);
    
        /* set the Tibia version */
        buf[0] = gzgetc(fpgzIn);
        buf[1] = gzgetc(fpgzIn);
        memcpy(&tibiaversion, &buf[0], 2);
    
        /* set the duration (in milliseconds) of the movie */
        buf[0] = gzgetc(fpgzIn);
        buf[1] = gzgetc(fpgzIn);
        buf[2] = gzgetc(fpgzIn);
        buf[3] = gzgetc(fpgzIn);
        memcpy(&msTotal, &buf[0], 4);
    
        msCombinedTotal += msTotal;
        
        while (1) {
            chunk = gzgetc(fpgzIn);
            
            if (chunk == EOF)
                break;
                
            if (chunk == RECORD_CHUNK_DATA) {
                /* get the delay to pause after the packet is sent (in milliseconds) */
                buf[0] = gzgetc(fpgzIn);
                buf[1] = gzgetc(fpgzIn);
                buf[2] = gzgetc(fpgzIn);
                buf[3] = gzgetc(fpgzIn);
                memcpy(&delay, &buf[0], 4);
    
                /* length of the packet */
                buf[0] = gzgetc(fpgzIn);
                buf[1] = gzgetc(fpgzIn);
                memcpy(&len, &buf[0], 2);
                
                gzread(fpgzIn, buf, len);
                
                fputc(RECORD_CHUNK_DATA, fpRecord);
                fwrite(&delay, 4, 1, fpRecord);
                fwrite(&len, 2, 1, fpRecord);
                fwrite(buf, len, 1, fpRecord);
            }
            else if (chunk == RECORD_CHUNK_MARKER) {
                fputc(RECORD_CHUNK_MARKER, fpRecord);
            }
        }
        gzclose(fpgzIn);
    }
    
    delay = 3000;
    len = 0;
    
    fputc(RECORD_CHUNK_DATA, fpRecord);
    fwrite(&delay, 4, 1, fpRecord);
    fwrite(&len, 2, 1, fpRecord);
    fwrite(nullbuf, len, 1, fpRecord);

    fseek(fpRecord, 0, SEEK_SET);
    fwrite(&version, 2, 1, fpRecord);
    fseek(fpRecord, 2, SEEK_SET);
    fwrite(&tibiaversion, 4, 1, fpRecord);
    fseek(fpRecord, 4, SEEK_SET);
    fwrite(&msCombinedTotal, 4, 1, fpRecord);
    fclose(fpRecord);
    sprintf(gzsaveFile, "%s.gz", outfile);
    
    fpRecord = fopen(outfile, "rb");
    fpgzRecord = gzopen(gzsaveFile, "wb");
    
    while (!feof(fpRecord)) {
        ret = fread(buf, 1, 16384, fpRecord);
        
        if (ret <= 0)
            break;
            
        gzwrite(fpgzRecord, buf, ret);
    }
    fclose(fpRecord);
    gzclose(fpgzRecord);
    remove(outfile);
    rename(gzsaveFile, outfile);
    fpRecord = NULL;
    
    return 1;
}

