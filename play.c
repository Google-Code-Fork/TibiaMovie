/*
 * play.c: movie playback
 *
 * Copyright 2004
 * See the COPYING file for more information on licensing and use.
 *
 * This file contains all functions relating to playing back a movie.
 */

#include <windows.h>
#include <winsock2.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <process.h>
#include "zlib.h"
#include "tibiamovie.h"

/* these variable names are embarassing :P */
int sockPlayListenCharacter = -1;
int sockPlayClientCharacter = -1;
int sockPlayListenServer    = -1;
int sockPlayClientServer    = -1;

char playFilename[512];
gzFile fpPlay = NULL;
int bytesPlayed = 0;
unsigned int msPlayed = 0;
unsigned int msTotal = 0;

int *playMarkers = NULL;
int playMarkersCnt = 0;

int playSpeed = 1;
int abortPlayThread = 0;
int fastForwarding = 0;
int playRec = 0;
int playing = 0;
int nextPacket = 0;
int numPackets = 0;
int curPacket = 0;

void PlayListen(int port, int *sock)
{
    struct hostent *host;
    struct sockaddr_in addr;

    *sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (!(host = gethostbyname("127.0.0.1"))) {
        return;
    }

    memcpy((char *)&addr.sin_addr, host->h_addr, host->h_length);
    addr.sin_family = host->h_addrtype;
    addr.sin_port = htons(port);

    if (bind(*sock, (struct sockaddr *)&addr, sizeof(struct sockaddr)) < 0) {
        return;
    }

    if (listen(*sock, 5) < 0) {
        return;
    }

    WSAAsyncSelect(*sock, wMain, WM_SOCKET_PLAY, FD_ACCEPT | FD_READ | FD_CLOSE);
    return;
}

void PlayStart(void)
{
    playing = 0;
    PlayListen(TIBIAPORT, &sockPlayListenCharacter);
    PlayListen(7172, &sockPlayListenServer);
    return;
}

void PlayEnd(void)
{
    closesocket(sockPlayListenCharacter);
    closesocket(sockPlayClientCharacter);
    closesocket(sockPlayListenServer);
    closesocket(sockPlayClientServer);
    playing = 0;
    
    /* if the play thread is still going, break on the next iteration */
    abortPlayThread = 1;
    return;
}

void PlayAccept(int fromsock, int *tosock)
{
    struct sockaddr_in addr;
    int temp = sizeof(struct sockaddr_in);

    if ((*tosock = accept(fromsock, (struct sockaddr *)&addr, &temp)) < 0)
        return;

    return;
}

struct filelist {
    char *filename;
    int version;
};

typedef struct filelist FILELIST;

int files_cmp(const void *x, const void *y)
{   
    FILELIST *dx = *(FILELIST **)x;
    FILELIST *dy = *(FILELIST **)y;
    
    char       *cdx;
    char       *cdy;
            
    cdx = dx->filename;
    cdy = dy->filename;
   
    return strcasecmp(cdx, cdy);
}

void PlaySendMotd(void)
{
    DIR *dir;
    struct dirent *e;
    int filecount = 0;
    char buf[10240];
    char motd[1024];
    int motdlen;
    int port = 7172;
    int premdays;
    int len;
    short l, l2;
    char *pos = buf;
    gzFile fp = NULL;
    FILELIST **files = NULL;
    int cnt;
    short version = 0, tibiaversion = 0;
    
    dir = opendir(".");
    while ((e = readdir(dir)) != 0) {
        l = strlen(e->d_name);
        if (l > 4 && (strcasecmp(e->d_name + l - 4, ".tmv") == 0 || strcasecmp(e->d_name + l - 4, ".rec") == 0)) {
            if ((fp = gzopen(e->d_name, "rb")) == NULL)
                continue;

            gzread(fp, &version, 2);
            gzread(fp, &tibiaversion, 2);
            gzclose(fp);
            
            if (compatmode && tibiaversion != TibiaVersionFound)
                continue;
                
            filecount++;
        }
    }
    closedir(dir);

    if (filecount > 0) {
        files = (FILELIST **)malloc(sizeof(FILELIST *) * filecount);
        memset(files, 0, sizeof(FILELIST *) * filecount);
        
        for (cnt = 0; cnt < filecount; cnt++)
            files[cnt] = (FILELIST *)malloc(sizeof(FILELIST));
    }

    premdays = filecount;
    
    cnt = 0;
    dir = opendir(".");
    while ((e = readdir(dir)) != 0) {
        l = strlen(e->d_name);
        if (l > 4 && (strcasecmp(e->d_name + l - 4, ".tmv") == 0 || strcasecmp(e->d_name + l - 4, ".rec") == 0)) {
            
            if ((fp = gzopen(e->d_name, "rb")) == NULL)
                continue;

            gzread(fp, &version, 2);
            gzread(fp, &tibiaversion, 2);
            gzclose(fp);

            if (compatmode && tibiaversion != TibiaVersionFound)
                continue;

            if (files[cnt]) {
                files[cnt]->filename = (char *)malloc(sizeof(char) * (strlen(e->d_name) + 1));
                strcpy(files[cnt]->filename, e->d_name);
                files[cnt]->version = tibiaversion;
                cnt++;
            }
        }
    }
    closedir(dir);

    qsort(files, filecount, sizeof(FILELIST *), files_cmp);
    
    /* fill in the first two bytes later on with the packet length */
    pos += 2;

    /* still don't know what this means */
    *pos++ = 0x14;

    /* send TibiaMovie's MOTD */
    sprintf(motd, "123\nWelcome to TibiaMovie!\n");
    motdlen = strlen(motd);
    memcpy(pos, &motdlen, 2); pos += 2;
    memcpy(pos, motd, motdlen); pos += motdlen;

    /* still don't know what this means */
    *pos++ = 0x64;

    /* number of "characters" (ie. files) we want to display to the client */
    *pos++ = filecount;

    /* display all files to the client */
    for (cnt = 0; cnt < filecount; cnt++) {
        short tibiaversion = 0;
        
        l = strlen(files[cnt]->filename);
        tibiaversion = files[cnt]->version;
        
        /* send the length of the filename */
        memcpy(pos, &l, 2); pos += 2;

        /* send the file itself */
        memcpy(pos, files[cnt]->filename, l); pos += l;

        /* if the tibia version is 0 for some reason, send a default string as the server name */
        if (tibiaversion == 0) {
            *pos++ = 10;
            *pos++ = 0x00;
            strcpy(pos, "TibiaMovie"); pos += 10;
        }
        else {
            char buf2[20];
                 
            sprintf(buf2, "%.02f", tibiaversion / 100.0);
            l2 = strlen(buf2);

            /* send the length of the server string */
            memcpy(pos, &l2, 2); pos += 2;

            /* send the server string */
            memcpy(pos, buf2, l2); pos += l2;
        }

        /* send the IP address (127.0.0.1) */
        *pos++ = 127; *pos++ = 0; *pos++ = 0; *pos++ = 1;

        /* send the port */
        memcpy(pos, &port, 2); pos += 2;
    }

    /* send the number of premium days remaining */
    memcpy(pos, &premdays, 2); pos += 2;

    /* buf = start of buffer, pos = end of buffer, so pos - buf is the length, and subtract
     * 2 for the first two bytes that we're about to set */
    len = pos - buf - 2;
    memcpy(&buf[0], &len, 2);

    /* finally! send the payload */
    send(sockPlayClientCharacter, buf, len + 2, 0);

    for (cnt = 0; cnt < filecount; cnt++) {
        if (files && files[cnt]) {
            free(files[cnt]->filename);
            free(files[cnt]);
        }
    }
    
    if (files)
        free(files);
        
    return;
}

void PlayFindMarkers(void)
{
    unsigned char buf[65500];
    short version, tibiaversion;
    int msPlayed = 0;
    int msTotal;
    int chunk;
    int delay;
    unsigned short len;
    gzFile fpFind;
    
    playMarkersCnt = 0;
    
    if (playMarkers) {
        free(playMarkers);
    }
    
    fpFind = gzopen(playFilename, "rb");
    /* set the TibiaMovie revision */
    buf[0] = gzgetc(fpFind);
    buf[1] = gzgetc(fpFind);
    memcpy(&version, &buf[0], 2);

    /* set the Tibia version */
    buf[0] = gzgetc(fpFind);
    buf[1] = gzgetc(fpFind);
    memcpy(&tibiaversion, &buf[0], 2);

    /* set the duration (in milliseconds) of the movie */
    buf[0] = gzgetc(fpFind);
    buf[1] = gzgetc(fpFind);
    buf[2] = gzgetc(fpFind);
    buf[3] = gzgetc(fpFind);
    memcpy(&msTotal, &buf[0], 4);

    while (1) {
        chunk = gzgetc(fpFind);
        
        if (chunk == EOF)
            break;
            
        if (chunk == RECORD_CHUNK_DATA) {
            numPackets++;
            /* get the delay to pause after the packet is sent (in milliseconds) */
            buf[0] = gzgetc(fpFind);
            buf[1] = gzgetc(fpFind);
            buf[2] = gzgetc(fpFind);
            buf[3] = gzgetc(fpFind);
            memcpy(&delay, &buf[0], 4);

            if (delay < 1000000)
                msPlayed += delay;
            
            /* length of the packet */
            buf[0] = gzgetc(fpFind);
            buf[1] = gzgetc(fpFind);
            memcpy(&len, &buf[0], 2);
            
            gzread(fpFind, buf, len);
        }
        else if (chunk == RECORD_CHUNK_MARKER) {
            if (!playMarkers) {
                playMarkers = (int *)malloc(sizeof(int) * 1);
            }
            else {
                playMarkers = (int *)realloc(playMarkers, sizeof(int) * (playMarkersCnt + 1));
            }
            
            playMarkers[playMarkersCnt] = msPlayed;
            playMarkersCnt++;
        }
    }
    
    gzclose(fpFind);
}

void Play(void *nothing)
{
    unsigned char buf[65500];
    unsigned int delay;
    unsigned short len;
    int llen;
    int ch;
    int quit = 0;
    int sent;
    int chunk;
    int numpacks = 0;
    short version;
    short tibiaversion;
    int c;

     len = strlen(playFilename);
     if (strcasecmp(playFilename + len - 4, ".rec") == 0)
         playRec = 1;
     else
          playRec = 0;

    numPackets = 0;
    curPacket = 0;
    
    if (!playRec) {
        PlayFindMarkers();
    }
    else {
        playMarkersCnt = 0;
    }
    
    /* do we need this? */
    /* Sleep(1000); */

    fpPlay = gzopen(playFilename, "rb");

    playing = 1;
    
    if (fpPlay) {
        if (!playRec) {
            /* set the TibiaMovie revision */
            buf[0] = gzgetc(fpPlay);
            buf[1] = gzgetc(fpPlay);
            memcpy(&version, &buf[0], 2);

            /* set the Tibia version */
            buf[0] = gzgetc(fpPlay);
            buf[1] = gzgetc(fpPlay);
            memcpy(&tibiaversion, &buf[0], 2);

            /* set the duration (in milliseconds) of the movie */
            buf[0] = gzgetc(fpPlay);
            buf[1] = gzgetc(fpPlay);
            buf[2] = gzgetc(fpPlay);
            buf[3] = gzgetc(fpPlay);
            memcpy(&msTotal, &buf[0], 4);
        }
        else {
            int seek = 0;
            /* don't really know what 0x03, 0x01 means? */
            buf[0] = gzgetc(fpPlay);
            buf[1] = gzgetc(fpPlay);
            memcpy(&version, &buf[0], 2);

            /* they store in seconds, so multiply by 1000 */
            buf[0] = gzgetc(fpPlay);
            buf[1] = gzgetc(fpPlay);
            buf[2] = gzgetc(fpPlay);
            buf[3] = gzgetc(fpPlay);
            memcpy(&numpacks, &buf[0], 4);

            seek = 6;
            
            for (c = 0; c < numpacks; c++) {
                buf[0] = gzgetc(fpPlay);
                buf[1] = gzgetc(fpPlay);
                buf[2] = gzgetc(fpPlay);
                buf[3] = gzgetc(fpPlay);
                seek += 4;
                memcpy(&llen, &buf[0], 4);
                len = (short)llen;
                     
                buf[0] = gzgetc(fpPlay);
                buf[1] = gzgetc(fpPlay);
                buf[2] = gzgetc(fpPlay);
                buf[3] = gzgetc(fpPlay);
                seek += 4;
                memcpy(&delay, &buf[0], 4);

                if (c == numpacks - 1) {
                    /* last pack */
                    msTotal = delay;
                    gzseek(fpPlay, 6, SEEK_SET);
                    break;
                }                
                seek += len;
                gzseek(fpPlay, seek, SEEK_SET);
            }
        }
                     
        while (abortPlayThread == 0) {
            int threadplaySpeed = playSpeed;
            int threadnextPacket = 0;
            int frame = 0;
            
            /* the word "CHUNK" is added to debug-style .tmv files, this ignores it */
            /* getc(fpPlay);getc(fpPlay);getc(fpPlay);getc(fpPlay);getc(fpPlay); */

            ch = gzgetc(fpPlay);

            /* check for EOF right off */
            if (ch == EOF)
                break;

            chunk = ch;

            if (chunk == RECORD_CHUNK_DATA || playRec) { /* if this is a data chunk */
                if (!playRec) {
                    /* get the delay to pause after the packet is sent (in milliseconds) */
                    buf[0] = gzgetc(fpPlay);
                    buf[1] = gzgetc(fpPlay);
                    buf[2] = gzgetc(fpPlay);
                    buf[3] = gzgetc(fpPlay);
                    memcpy(&delay, &buf[0], 4);

                    /* length of the packet */
                    buf[0] = gzgetc(fpPlay);
                    buf[1] = gzgetc(fpPlay);
                    memcpy(&len, &buf[0], 2);
                }
                else {
                    buf[0] = ch;
                    buf[1] = gzgetc(fpPlay);
                    buf[2] = gzgetc(fpPlay);
                    buf[3] = gzgetc(fpPlay);
                    memcpy(&llen, &buf[0], 4);
                    len = (short)llen;
                     
                    buf[0] = gzgetc(fpPlay);
                    buf[1] = gzgetc(fpPlay);
                    buf[2] = gzgetc(fpPlay);
                    buf[3] = gzgetc(fpPlay);
                    memcpy(&delay, &buf[0], 4);
                    delay = delay - msPlayed;
                }

                for (c = 0; c < len && c < 30000; c++) {
                    ch = gzgetc(fpPlay);
                    
                    if (ch == EOF) {
                        quit = 1;
                        break;
                    }
                    /* don't know what i was smoking when i did this, but it sure is ugly! */
                    else if (ch == '&') {
                        ch = gzgetc(fpPlay);
                        if (ch == 'm') {
                            ch = gzgetc(fpPlay);
                            if (ch == 'i') {
                                ch = gzgetc(fpPlay);
                                if (ch == 'd') {
                                    ch = gzgetc(fpPlay);
                                    if (ch == 'd') {
                                        ch = gzgetc(fpPlay);
                                        if (ch == 'o') {
                                            ch = gzgetc(fpPlay);
                                            if (ch == 't') {
                                                ch = gzgetc(fpPlay);
                                                if (ch == ';') {
                                                    ch = 0xb7;
                                                }
                                                else { gzseek(fpPlay, gztell(fpPlay) - 7, SEEK_SET); ch = '&'; }
                                            }
                                            else { gzseek(fpPlay, gztell(fpPlay) - 6, SEEK_SET); ch = '&'; }
                                        }
                                        else { gzseek(fpPlay, gztell(fpPlay) - 5, SEEK_SET); ch = '&'; }
                                    }
                                    else { gzseek(fpPlay, gztell(fpPlay) - 4, SEEK_SET); ch = '&'; }
                                }
                                else { gzseek(fpPlay, gztell(fpPlay) - 3, SEEK_SET); ch = '&'; }
                            }
                            else { gzseek(fpPlay, gztell(fpPlay) - 2, SEEK_SET); ch = '&'; }
                        }
                        else {
                            gzseek(fpPlay, gztell(fpPlay) - 1, SEEK_SET);
                            ch = '&';
                        }
                    }
                    buf[c] = ch;
                }

                /* EOF was detected whilst looping the packet, or the delay was too big..
                 * 15 minutes (900 seconds) in milliseconds is 900,000 ms, so a delay of
                 * a million or above just cannot happen due to timeouts, i hope :P */
                if (quit || delay > 1000000)
                    break;

                if (playSpeed == 0) {
                    /* simulate a "pause" effect */
                    while (playSpeed == 0 && mode == MODE_PLAY && nextPacket == 0) {
                        /* this is safe, we're in our own thread! */
                        Sleep(100);
                    }
                }

                threadplaySpeed = playSpeed;
                threadnextPacket = nextPacket;
                
#define DELAY_UPDATE 1000

                if (threadnextPacket) {
                    nextPacket = 0;
                    threadnextPacket = 0;
                    msPlayed += delay;
                    frame = 1;
                }
                else if (threadplaySpeed == 0) {
                    break;
                }
                else if (delay / threadplaySpeed < DELAY_UPDATE) {
                        Sleep(delay / UMAX(1, threadplaySpeed));
                        msPlayed += delay;
                }
                else {
                    int ndelay = delay;
                    
                    while (ndelay >= DELAY_UPDATE) {
                        threadplaySpeed = playSpeed;

                        if (threadplaySpeed == 0)
                            while (playSpeed == 0 && nextPacket == 0 && mode == MODE_PLAY) {
                                /* this is safe, we're in our own thread! */
                                Sleep(100);
                            }

                        threadplaySpeed = playSpeed;
                        threadnextPacket = nextPacket;
                        
                        if (threadnextPacket) {
                            nextPacket = 0;
                            break;
                        }
                        else {
                            ndelay -= DELAY_UPDATE;
                            Sleep(DELAY_UPDATE / UMAX(1, threadplaySpeed));
                            msPlayed += DELAY_UPDATE;
                        }
                        
                        InvalidateRect(wMain, NULL, TRUE);
                    }
                    
                    if (ndelay > 0) {
                        if (threadnextPacket == 0)
                            Sleep(ndelay / UMAX(1, threadplaySpeed));
                            
                        msPlayed += ndelay;
                    }
                }
                
                sent = send(sockPlayClientServer, buf, len, 0);
                curPacket++;
                
                if (sent == -1)
                    break;
                    
                /* first packet, insert our own status message */
                if (bytesPlayed == 0 && playRec == 0) {
                    short mylen;
                    char msgbuf[512];
                    
                    memcpy(&mylen, &buf[0], 2);
                    
                    /* whilst this interferes with the tibia protocol by injecting our
                     * own message (harmless anyway), this should still be legal as
                     * since it's only in playback it doesn't interact with the tibia
                     * servers whatsoever.
                     */
                    if (mylen == len - 2) {
                        sprintf(msgbuf, "Recorded in Tibia %.02f using TibiaMovie (%d). "
                                        "Get TibiaMovie at tibiamovie.sourceforge.net/",
                                        tibiaversion / 100.0, version);
                        SendStatusMessage(sockPlayClientServer, msgbuf);
                        bytesPlayed += 13;
                    }
                }

                bytesPlayed += len + 7;

                /* update "bytes played" in the window, but don't update too often otherwise we get flickering */
                if (GetTickCount() > lastDraw + 200 || frame) {
                    InvalidateRect(wMain, NULL, TRUE);
                    lastDraw = GetTickCount();
                }
            } /* end data chunk check */
            else if (chunk == RECORD_CHUNK_MARKER) {
                if (fastForwarding == 1) {
                    /* marker detected, reset the play speed if the button was pressed */
                    playSpeed = 1;
                    fastForwarding = 0;
                    EnableWindow(btnGoToMarker, 1);
                }

                bytesPlayed++;
            }
        }

        gzclose(fpPlay);
    }

    playing = 0;
    msPlayed = msTotal;
    InvalidateRect(wMain, NULL, TRUE);
    closesocket(sockPlayClientServer);
    _endthread();
    return;
}

void DoSocketPlay(HWND hwnd, int wEvent, int wError, int sock)
{
    unsigned char buf[65535];
    int cnt = 0;
    
    if (sock == sockPlayListenCharacter && wEvent == FD_ACCEPT) {
        PlayAccept(sock, &sockPlayClientCharacter);
        PlaySendMotd();
        return;
    }

    if (sock == sockPlayListenServer && wEvent == FD_ACCEPT) {
        bytesPlayed = 0;
        abortPlayThread = 0;
        msPlayed = 0;
        msTotal = 0;
        fastForwarding = 0;
        playFilename[0] = '\0';
        PlayAccept(sock, &sockPlayClientServer);
        return;
    
    }

    if (sock == sockPlayClientCharacter && wEvent == FD_READ) {
        while (recv(sock, buf, 4096, 0) > 0);
        return;
    }

    if (sock == sockPlayClientServer && wEvent == FD_READ) {
        short filelen;
        char filebuf[512];
        
        while ((cnt = recv(sock, buf, 4096, 0)) > 0) {
            /* only want the first packet, ie. the filename, so make sure nothing has been
             * played yet, assuring this IS the first packet */
            if (bytesPlayed == 0 && cnt > 16) {
                ZeroMemory(filebuf, sizeof(filebuf));
                memcpy(&filelen, &buf[12], 2);
                memcpy(filebuf, &buf[14], filelen);
                strcpy(playFilename, filebuf);
                _beginthread(Play, 0, NULL);
            }
        }
    }

    return;
}

void SendStatusMessage(int sock, char *message)
{
    char out[1024];
    char *pout = out;
    short packlen = 0;
    short messagelen = 0;

    messagelen = strlen(message);
    packlen = messagelen + 4;
    memcpy(pout, &packlen, 2);
    pout += 2;
    *pout++ = 0xb4;
    *pout++ = 0x14;
    memcpy(pout, &messagelen, 2);
    pout += 2;
    memcpy(pout, message, messagelen);
    send(sock, out, packlen + 2, 0);
}

char *duration(unsigned int dur, char *dest)
{
    char *dst = dest;
    char buf[5];
    int  d, h, m, s;

    *dest = 0;

    s = dur % 60;
    m = (dur / 60) % 60;
    h = (dur / 60 / 60) % 24;
    d = (dur / 60 / 60 / 24);

    if (d) {
        sprintf(buf, "%dd ", d);
        strcat(dst, buf);
    }
    if (h) {
        sprintf(buf, "%dh ", h);
        strcat(dst, buf);
    }
    if (m) {
        sprintf(buf, "%dm ", m);
        strcat(dst, buf);
    }
    if (s) {
        sprintf(buf, "%ds ", s);
        strcat(dst, buf);
    }

    while (*++dst);

    dst--;
    *dst = 0;

    return dest;
}
