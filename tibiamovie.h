/*
 * tibiamovie.h: duh?
 *
 * Copyright 2004
 * See the COPYING file for more information on licensing and use.
 *
 */

#define MOVIEVERSION 1
#define TIBIAPORT 7171

#define WM_SOCKET_RECORD (WM_USER + 1)
#define WM_SOCKET_PLAY   (WM_USER + 2)

#define RECORD_CHUNK_DATA   0x00
#define RECORD_CHUNK_MARKER 0x01

extern HWND wMain;
extern HWND btnRecord;
extern HWND btnServers;

extern char saveFile[512];

extern int memorySaveCnt;
extern int debug;

/* proxy.c */
void ProxyAccept(HWND hwnd, int socket);
void ProxyHandleServer(HWND hwnd, char *buf);

extern int TibiaVersionFound;

//extern FILE *fp;
extern FILE *fpRecord;

#define MODE_NONE         0
#define MODE_PLAY         1
#define MODE_RECORD       2
#define MODE_RECORD_PAUSE 3
extern int mode;

extern unsigned int recordStart;

#define UMIN(a, b)           ((a) < (b) ? (a) : (b))
#define UMAX(a, b)           ((a) > (b) ? (a) : (b))


/* play.c */
extern int sockPlayListenCharacter;
extern int sockPlayClientCharacter;
extern int sockPlayListenServer;
extern int sockPlayClientServer;

extern char playFilename[512];
extern FILE *fpPlay;
extern int bytesPlayed;
extern int playSpeed;
extern int abortPlayThread;
extern unsigned int msPlayed;
extern unsigned int msTotal;
extern int fastForwarding;

void PlayListen(int port, int *sock);
void PlayStart(void);
void PlayEnd(void);
void PlayAccept(int fromsock, int *tosock);
void PlaySendMotd(void);
void Play(void* nothing);
void DoSocketPlay(HWND hwnd, int wEvent, int wError, int sock);
void SendStatusMessage(int sock, char *message);
char *duration(unsigned int dur, char *dest);
/* end play.c */

/* memory.c */
extern int memoryActivated;

void MemoryInjectionSearch(int toggle);
void MemoryInjection(int toggle);
int AdjustPrivileges(void);
/* end memory.c */

/* main.c */
extern int mode;
void FindUnusedMovieName(void);
void debugf(char *fmt, ...) __attribute__((format(printf, 1, 2)));
/* end main.c */

/* record.c */
struct serverData
{
    unsigned long ip;
    short port;
    char characterName[128];
    short characterLength;
};

extern struct serverData servers[1000];
extern int bytesRecorded;
extern int numMarkers;
extern char *loginservers[];

void RecordFillServerBox(void);
void RecordData(unsigned char *buf, short len);
void RecordStart(void);
void RecordEnd(void);
void RecordDisconnect(void);
void DoSocketRecord(HWND hwnd, int wEvent, int wError, int sock);
void RecordAddMarker(void);
/* end record.c */
