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

#include "tibiamovie.h"

/* these variable names are embarassing :P */
int sockPlayListenCharacter = -1;
int sockPlayClientCharacter = -1;
int sockPlayListenServer    = -1;
int sockPlayClientServer    = -1;

char playFilename[512];
FILE *fpPlay = NULL;
int bytesPlayed = 0;
unsigned int msPlayed = 0;
unsigned int msTotal = 0;

int playSpeed = 1;
int abortPlayThread = 0;
int fastForwarding = 0;
int playRec = 0;

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

void PlaySendMotd(void)
{
    DIR *dir;
    struct dirent *e;
    int filecount = 0;
    char buf[10240];
    char motd[1024];
    int motdlen;
    int port = 7172;
    int premdays = 1337;
    int len;
    short l, l2;
    char *pos = buf;
    FILE *fp = NULL;
    

    dir = opendir(".");
    while ((e = readdir(dir)) != 0) {
        l = strlen(e->d_name);
        if (l > 4 && (strcasecmp(e->d_name + l - 4, ".tmv") == 0 || strcasecmp(e->d_name + l - 4, ".rec") == 0)) {
            if ((fp = fopen(e->d_name, "r")) == NULL)
                continue;

            fclose(fp);
            filecount++;
        }
    }
    closedir(dir);

    /* TODO: do we need this? */
    /* Sleep(1000); */

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
    dir = opendir(".");
    while ((e = readdir(dir)) != 0) {
        l = strlen(e->d_name);
        if (l > 4 && (strcasecmp(e->d_name + l - 4, ".tmv") == 0 || strcasecmp(e->d_name + l - 4, ".rec") == 0)) {
            short version = 0, tibiaversion = 0;
            int playRec;
            
            if ((fp = fopen(e->d_name, "r")) == NULL)
                 continue;

            if (strcasecmp(e->d_name + l - 4, ".rec") == 0)
                playRec = 1;
            else
                playRec = 0;
                
            fread(&version, 2, 1, fp);
            fread(&tibiaversion, 2, 1, fp);
            fclose(fp);

            if (playRec)
                tibiaversion = version;
                
            /* send the length of the filename */
            memcpy(pos, &l, 2); pos += 2;

            /* send the file itself */
            memcpy(pos, e->d_name, l); pos += l;

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
    }
    closedir(dir);

    /* send the number of premium days remaining */
    memcpy(pos, &premdays, 2); pos += 2;

    /* buf = start of buffer, pos = end of buffer, so pos - buf is the length, and subtract
     * 2 for the first two bytes that we're about to set */
    len = pos - buf - 2;
    memcpy(&buf[0], &len, 2);

    /* finally! send the payload */
    send(sockPlayClientCharacter, buf, len + 2, 0);

    return;
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

    /* do we need this? */
    /* Sleep(1000); */

    fpPlay = fopen(playFilename, "rb");

    if (fpPlay) {
        if (!playRec) {
            /* set the TibiaMovie revision */
            buf[0] = getc(fpPlay);
            buf[1] = getc(fpPlay);
            memcpy(&version, &buf[0], 2);

            /* set the Tibia version */
            buf[0] = getc(fpPlay);
            buf[1] = getc(fpPlay);
            memcpy(&tibiaversion, &buf[0], 2);

            /* set the duration (in milliseconds) of the movie */
            buf[0] = getc(fpPlay);
            buf[1] = getc(fpPlay);
            buf[2] = getc(fpPlay);
            buf[3] = getc(fpPlay);
            memcpy(&msTotal, &buf[0], 4);
        }
        else {
            int seek = 0;
            /* don't really know what 0x03, 0x01 means? */
            buf[0] = getc(fpPlay);
            buf[1] = getc(fpPlay);
            memcpy(&version, &buf[0], 2);

            /* they store in seconds, so multiply by 1000 */
            buf[0] = getc(fpPlay);
            buf[1] = getc(fpPlay);
            buf[2] = getc(fpPlay);
            buf[3] = getc(fpPlay);
            memcpy(&numpacks, &buf[0], 4);

            seek = 6;
            
            for (c = 0; c < numpacks; c++) {
                buf[0] = getc(fpPlay);
                buf[1] = getc(fpPlay);
                buf[2] = getc(fpPlay);
                buf[3] = getc(fpPlay);
                seek += 4;
                memcpy(&llen, &buf[0], 4);
                len = (short)llen;
                     
                buf[0] = getc(fpPlay);
                buf[1] = getc(fpPlay);
                buf[2] = getc(fpPlay);
                buf[3] = getc(fpPlay);
                seek += 4;
                memcpy(&delay, &buf[0], 4);

                if (c == numpacks - 1) {
                    /* last pack */
                    msTotal = delay;
                    fseek(fpPlay, 6, SEEK_SET);
                    break;
                }                
                seek += len;
                fseek(fpPlay, seek, SEEK_SET);
            }
        }
                     
        while (abortPlayThread == 0) {
            /* the word "CHUNK" is added to debug-style .tmv files, this ignores it */
            /* getc(fpPlay);getc(fpPlay);getc(fpPlay);getc(fpPlay);getc(fpPlay); */

            ch = getc(fpPlay);

            /* check for EOF right off */
            if (ch == EOF)
                break;

            chunk = ch;

            if (chunk == RECORD_CHUNK_DATA || playRec) { /* if this is a data chunk */
                if (!playRec) {
                    /* get the delay to pause after the packet is sent (in milliseconds) */
                    buf[0] = getc(fpPlay);
                    buf[1] = getc(fpPlay);
                    buf[2] = getc(fpPlay);
                    buf[3] = getc(fpPlay);
                    memcpy(&delay, &buf[0], 4);

                    /* length of the packet */
                    buf[0] = getc(fpPlay);
                    buf[1] = getc(fpPlay);
                    memcpy(&len, &buf[0], 2);
                }
                else {
                    buf[0] = ch;
                    buf[1] = getc(fpPlay);
                    buf[2] = getc(fpPlay);
                    buf[3] = getc(fpPlay);
                    memcpy(&llen, &buf[0], 4);
                    len = (short)llen;
                     
                    buf[0] = getc(fpPlay);
                    buf[1] = getc(fpPlay);
                    buf[2] = getc(fpPlay);
                    buf[3] = getc(fpPlay);
                    memcpy(&delay, &buf[0], 4);
                    delay = delay - msPlayed;
                }

                for (c = 0; c < len && c < 30000; c++) {
                    ch = getc(fpPlay);
                    
                    if (ch == EOF) {
                        quit = 1;
                        break;
                    }
                    /* don't know what i was smoking when i did this, but it sure is ugly! */
                    else if (ch == '&') {
                        ch = getc(fpPlay);
                        if (ch == 'm') {
                            ch = getc(fpPlay);
                            if (ch == 'i') {
                                ch = getc(fpPlay);
                                if (ch == 'd') {
                                    ch = getc(fpPlay);
                                    if (ch == 'd') {
                                        ch = getc(fpPlay);
                                        if (ch == 'o') {
                                            ch = getc(fpPlay);
                                            if (ch == 't') {
                                                ch = getc(fpPlay);
                                                if (ch == ';') {
                                                    ch = 0xb7;
                                                }
                                                else { ungetc(ch, fpPlay);ungetc('t', fpPlay);ungetc('o', fpPlay);ungetc('d', fpPlay);ungetc('d', fpPlay);ungetc('i', fpPlay);ungetc('m', fpPlay);ch = '&'; }
                                            }
                                            else { ungetc(ch, fpPlay);ungetc('o', fpPlay);ungetc('d', fpPlay);ungetc('d', fpPlay);ungetc('i', fpPlay);ungetc('m', fpPlay);ch = '&'; }
                                        }
                                        else { ungetc(ch, fpPlay);ungetc('d', fpPlay);ungetc('d', fpPlay);ungetc('i', fpPlay);ungetc('m', fpPlay);ch = '&'; }
                                    }
                                    else { ungetc(ch, fpPlay);ungetc('d', fpPlay);ungetc('i', fpPlay);ungetc('m', fpPlay);ch = '&'; }
                                }
                                else { ungetc(ch, fpPlay);ungetc('i', fpPlay);ungetc('m', fpPlay);ch = '&'; }
                            }
                            else { ungetc(ch, fpPlay);ungetc('m', fpPlay);ch = '&'; }
                        }
                        else {
                            ungetc(ch, fpPlay);
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
                    while (playSpeed == 0 && mode == MODE_PLAY) {
                        /* this is safe, we're in our own thread! */
                        Sleep(100);
                    }
                }

#define DELAY_UPDATE 1000

                /* delay for delay milliseconds, depending on speed */
                if (delay / playSpeed < DELAY_UPDATE) {
                    Sleep(delay / playSpeed);
                    msPlayed += delay;
                }
                else {
                    int ndelay = delay;
                    
                    while (ndelay >= DELAY_UPDATE) {
                        ndelay -= DELAY_UPDATE;
                        Sleep(DELAY_UPDATE / playSpeed);
                        msPlayed += DELAY_UPDATE;
                        InvalidateRect(wMain, NULL, TRUE);
                    }
                    
                    if (ndelay > 0) {
                        Sleep(ndelay / playSpeed);
                        msPlayed += ndelay;
                    }
                }
                
                sent = send(sockPlayClientServer, buf, len, 0);
                
                if (sent == -1)
                    break;
                    
                /* first packet, insert our own status message */
                if (bytesPlayed == 0 && playRec == 0) {
                    char msgbuf[512];
                    sprintf(msgbuf, "Recorded in Tibia %.02f using TibiaMovie %d. "
                                    "Get TibiaMovie at tibiamovie.sourceforge.net/",
                                    tibiaversion / 100.0, version);
                    SendStatusMessage(sockPlayClientServer, msgbuf);
                    bytesPlayed += 13;
                }

                bytesPlayed += len + 7;

                /* update "bytes played" in the window */
                InvalidateRect(wMain, NULL, TRUE);
            } /* end data chunk check */
            else if (chunk == RECORD_CHUNK_MARKER) {
                if (fastForwarding == 1) {
                    /* marker detected, reset the play speed if the button was pressed */
                    playSpeed = 1;
                    fastForwarding = 0;
                }

                bytesPlayed++;
            }
        }

        fclose(fpPlay);
    }

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
