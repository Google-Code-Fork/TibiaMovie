/*
 * record.c: movie recording
 *
 * Copyright 2004
 * See the COPYING file for more information on licensing and use.
 *
 * This file contains all functions relating to recording a movie.
 */


#include <windows.h>
#include <winsock2.h>
#include <stdio.h>
#include <process.h>

#include "tibiamovie.h"

struct serverData servers[1000];
int serverspos = 0;

/* these variable names are embarassing :P */
int sockRecordListenCharacter  = -1;
int sockRecordClientCharacter  = -1;
int sockRecordConnectCharacter = -1;
int sockRecordConnectCharacterConnected = 0;

char characterQueueBuf[1024];
char *characterQueuePos = NULL;

int sockRecordListenServer     = -1;
int sockRecordClientServer     = -1;
int sockRecordConnectServer    = -1;
int sockRecordConnectServerConnected = 0;
char serverQueueBuf[1024];
char *serverQueuePos = NULL;

FILE *fpRecord = NULL;
unsigned int recordStart = 0;
int recordTotal = 0;
int bytesRecorded = 0;
int numMarkers = 0;


char *loginservers[] =
    { "tibia1.cipsoft.com",
      "tibia2.cipsoft.com",
      "server.tibia.com",
      "server2.tibia.com",
      "test.cipsoft.com",
      NULL
    };

void RecordMotdModify(char *buf);
void RecordData(unsigned char *buf, short len);

void RecordFillServerBox(void)
{
    int c;

    for (c = 0; loginservers[c] != NULL; c++)
        SendMessage(btnServers, LB_ADDSTRING, 0, (LPARAM)loginservers[c]);

    return;
}

void RecordListen(int port, int *sock)
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

    WSAAsyncSelect(*sock, wMain, WM_SOCKET_RECORD, FD_ACCEPT | FD_READ | FD_CLOSE);
    return;
}

void RecordStart(void)
{
    sockRecordConnectCharacterConnected = 0;
    sockRecordConnectServerConnected = 0;
    bytesRecorded = 0;
    numMarkers = 0;
    characterQueuePos = characterQueueBuf;
    serverQueuePos = serverQueueBuf;

    RecordListen(TIBIAPORT, &sockRecordListenCharacter);
    RecordListen(7172, &sockRecordListenServer);
}

void RecordEnd(void)
{
    if (fpRecord) {
        char nullbuf[2] = {0, 0};
        unsigned int delay;
        short len = 0;
        delay = timeGetTime() - recordStart;
        recordTotal += delay;
        fputc(RECORD_CHUNK_DATA, fpRecord);
        fwrite(&delay, 4, 1, fpRecord);
        fwrite(&len, 2, 1, fpRecord);
        fwrite(nullbuf, len, 1, fpRecord);

        fseek(fpRecord, 4, SEEK_SET);
        fwrite(&recordTotal, 4, 1, fpRecord);
        fclose(fpRecord);
        fpRecord = NULL;
    }

    bytesRecorded = 0;
    recordTotal = 0;
    numMarkers = 0;
    characterQueuePos = characterQueueBuf;
    serverQueuePos = serverQueueBuf;

    FindUnusedMovieName();
    mode = MODE_RECORD_PAUSE;
    SetWindowText(btnRecord, "Disconnect");
    InvalidateRect(wMain, NULL, TRUE);
    return;
}

void RecordDisconnect(void)
{
    closesocket(sockRecordListenCharacter);
    closesocket(sockRecordClientCharacter);
    closesocket(sockRecordConnectCharacter);

    closesocket(sockRecordListenServer);
    closesocket(sockRecordClientServer);
    closesocket(sockRecordConnectServer);

    return;
}

void RecordAccept(int fromsock, int *tosock)
{
    struct sockaddr_in addr;
    int temp = sizeof(struct sockaddr_in);

    if ((*tosock = accept(fromsock, (struct sockaddr *)&addr, &temp)) < 0)
        return;

    return;
}

int RecordConnect(int *sock, char *host, short port)
{
    struct hostent *hoste;
    struct sockaddr_in addr;

    ZeroMemory(&addr, sizeof(struct sockaddr_in));
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;

    if (!(hoste = gethostbyname(host)))
        return 0;

    memcpy((char *)&addr.sin_addr, hoste->h_addr, hoste->h_length);
    addr.sin_family = hoste->h_addrtype;
    addr.sin_port = htons(TIBIAPORT);

    *sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    WSAAsyncSelect(*sock, wMain, WM_SOCKET_RECORD, FD_CONNECT | FD_READ | FD_CLOSE);

    if (connect(*sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0)
        return 0;

    return 1;
}

void DoSocketRecord(HWND hwnd, int wEvent, int wError, int sock)
{
    unsigned char buf[65535];
    int cnt = 0;

    if (wEvent == FD_CLOSE
        && (sock == sockRecordClientCharacter || sock == sockRecordConnectCharacter)
       ) {
        closesocket(sockRecordClientCharacter);
        closesocket(sockRecordConnectCharacter);
        sockRecordConnectCharacterConnected = 0;
        return;
    }

    if (wEvent == FD_CLOSE
        && (sock == sockRecordClientServer || sock == sockRecordConnectServer)
       ) {
        closesocket(sockRecordClientServer);
        closesocket(sockRecordConnectServer);
        sockRecordConnectServerConnected = 0;
        RecordEnd();
        RecordDisconnect();
        return;
    }

    if (sock == sockRecordListenCharacter && wEvent == FD_ACCEPT) {
        cnt = SendMessage(btnServers, LB_GETCURSEL, 0, 0);

        /* act as a proxy for the "character list" */
        RecordAccept(sock, &sockRecordClientCharacter);
        RecordConnect(&sockRecordConnectCharacter, loginservers[cnt], TIBIAPORT);
        return;
    }

    if (sock == sockRecordClientCharacter && wEvent == FD_READ) {
        while ((cnt = recv(sock, buf, 65530, 0)) > 0) {
            /* haven't connected to the server yet, but receiving data from client,
             * so put it in a queue and send it when we finally connect */
            if (sockRecordConnectCharacterConnected == 0) {
                memcpy(characterQueuePos, buf, cnt);
                characterQueuePos += cnt;
            }
            else {
                send(sockRecordConnectCharacter, buf, cnt, 0);
            }
        }

        return;
    }

    if (sock == sockRecordConnectCharacter && wEvent == FD_CONNECT) {
        if (characterQueuePos != characterQueueBuf) {
            /* characterQueue isn't empty, send it to the server, then purge it */
            send(sockRecordConnectCharacter, characterQueueBuf, characterQueuePos - characterQueueBuf, 0);
            characterQueueBuf[0] = '\0';
            characterQueuePos = characterQueueBuf;
        }

        sockRecordConnectCharacterConnected = 1;
        return;
    }

    if (sock == sockRecordConnectCharacter && wEvent == FD_READ) {
        while ((cnt = recv(sock, buf, 65530, 0)) > 0) {
            if (sockRecordClientCharacter == -1) {
                /* we have data from the server, but the client doesn't exist? eek. */
                closesocket(sockRecordConnectCharacter);
            }
            else {
                RecordMotdModify(buf);
                send(sockRecordClientCharacter, buf, cnt, 0);
            }
        }

        return;
    }

    if (sock == sockRecordListenServer && wEvent == FD_ACCEPT) {
        /* act as a proxy for the server connection */
        RecordAccept(sock, &sockRecordClientServer);
        return;
    }

    if (sock == sockRecordClientServer && wEvent == FD_READ) {
        while ((cnt = recv(sock, buf, 65530, 0)) > 0) {
            /* haven't connected to the server yet, but receiving data from client,
             * so put it in a queue and send it when we finally connect, and also parse it
             * for a character name so we know which server to connect to! */
            if (sockRecordConnectServerConnected == 0) {
                memcpy(serverQueuePos, buf, cnt);
                serverQueuePos += cnt;

                if (cnt > 20) {
                    short namelen;
                    char name[256];
                    char ip[4];
                    char ipbuf[32];
                    int c;

                    ZeroMemory(name, sizeof(name));
                    memcpy(&namelen, &buf[12], 2);
                    memcpy(name, &buf[14], namelen);

                    for (c = 0; servers[c].ip != 0; c++) {
                        if (memcmp(name, servers[c].characterName, namelen) == 0)
                            break;
                    }
                    
                    if (servers[c].ip != 0) {
                        memcpy(ip, &servers[c].ip, 4);
                        sprintf(ipbuf, "%d.%d.%d.%d", (unsigned char)ip[0], (unsigned char)ip[1], (unsigned char)ip[2], (unsigned char)ip[3]);
                        RecordConnect(&sockRecordConnectServer, ipbuf, servers[c].port);
                    }
                }
            }
            else {
                send(sockRecordConnectServer, buf, cnt, 0);
            }
        }

        return;
    }

    if (sock == sockRecordConnectServer && wEvent == FD_CONNECT) {
        if (serverQueuePos != serverQueueBuf) {
            /* serverQueue isn't empty, send it to the server, then purge it */
            send(sockRecordConnectServer, serverQueueBuf, serverQueuePos - serverQueueBuf, 0);
            serverQueueBuf[0] = '\0';
            serverQueuePos = serverQueueBuf;
        }

        sockRecordConnectServerConnected = 1;
        return;
    }

    if (sock == sockRecordConnectServer && wEvent == FD_READ) {
        while ((cnt = recv(sock, buf, 65530, 0)) > 0) {
            if (sockRecordClientServer == -1) {
                /* we have data from the server, but the client doesn't exist? eek. */
                closesocket(sockRecordConnectServer);
            }
            else {
                if (mode == MODE_RECORD)
                    RecordData(buf, cnt);
                    
                send(sockRecordClientServer, buf, cnt, 0);
            }
        }

        return;
    }

    return;
}

void RecordMotdModify(char *buf)
{
    char *out = buf;
    short len, motdlen;
    short numchars;
    int cnt;
    int premdays;
    int localport = 7172;

    memcpy(&len, out, 2);
    out += 2;

    if (*out != 0x14)
        return;

    ZeroMemory(servers, sizeof(servers));
    serverspos = 0;

    out++;

    memcpy(&motdlen, out, 2);  out +=2;
    out += motdlen;
    out++;

    numchars = *out++;
    
    for (cnt = 0; cnt < numchars; cnt++) {
        short namelen, serverlen;
        char namebuf[512], serverbuf[512];
        int localip = ntohl(2130706433);
        int ip;
        int port;
        
        memcpy(&namelen, out, 2); out += 2;
        memcpy(&namebuf, out, namelen); out += namelen;
        namebuf[namelen] = 0;
        memcpy(&serverlen, out, 2); out += 2;
        memcpy(&serverbuf, out, serverlen); out += serverlen;
        serverbuf[serverlen] = 0;
        memcpy(&ip, out, 4);
        memcpy(out, &localip, 4);
        out += 4;
        memcpy(&port, out, 2);
        memcpy(out, &localport, 2);
        out += 2;
        
        servers[serverspos].ip = ip;
        servers[serverspos].port = port;
        memcpy(servers[serverspos].characterName, namebuf, namelen);
        servers[serverspos].characterLength = namelen;

        serverspos++;
    }

    memcpy(&premdays, out, 2);

    return;
}

void RecordData(unsigned char *buf, short len)
{
    unsigned int delay = 0;
    short version = MOVIEVERSION;
    short tibiaversion = (short)TibiaVersionFound;
  
    if (len == 0)
        return;

    if (len > 10 && buf[2] == 0x14) {
        send(sockRecordClientServer, buf, len, 0);
        Sleep(1000);
        closesocket(sockRecordClientServer);
        closesocket(sockRecordConnectServer);
        return;
    }

    if (!fpRecord) {
        unsigned int secondsElapsed = 0;

        recordTotal = 0;
        FindUnusedMovieName();
        fpRecord = fopen(saveFile, "wb");
        recordStart = timeGetTime();
        fwrite(&version, 2, 1, fpRecord);
        fwrite(&tibiaversion, 2, 1, fpRecord);
        fwrite(&secondsElapsed, 4, 1, fpRecord);
    }

    delay = timeGetTime() - recordStart;
    recordStart = timeGetTime();
    recordTotal += delay;

    fputc(RECORD_CHUNK_DATA, fpRecord);
    fwrite(&delay, 4, 1, fpRecord);
    fwrite(&len, 2, 1, fpRecord);
    fwrite(buf, len, 1, fpRecord);

    bytesRecorded += len + 5 + 6;

    InvalidateRect(wMain, NULL, TRUE);
    return;
}

void RecordAddMarker(void)
{
    if (!fpRecord)
        return;

    fputc(RECORD_CHUNK_MARKER, fpRecord);
    bytesRecorded += 1;
    numMarkers++;
    return;
}

