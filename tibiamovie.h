/*
 * tibiamovie.h: duh?
 *
 * Copyright 2004
 * See the COPYING file for more information on licensing and use.
 *
 */

#define TIBIAMOVIE_VERSION "0.2.3"
#define MOVIEVERSION 2
#define TIBIAPORT 7171

#define WM_SOCKET_RECORD (WM_USER + 1)
#define WM_SOCKET_PLAY   (WM_USER + 2)

#define RECORD_CHUNK_DATA   0x00
#define RECORD_CHUNK_MARKER 0x01

#define MENU_FILE_EXIT           300
#define MENU_OPTIONS_COMPAT_MODE 301
#define MENU_HELP_ABOUT          302

extern HWND wMain;
extern HWND btnRecord;
extern HWND btnServers;
extern HWND btnAddMarker;
extern HWND btnGoToMarker;

extern char saveFile[512];

extern int memorySaveCnt;
extern int debug;
extern int compatmode;

extern unsigned long int lastDraw;

/* proxy.c */
void ProxyAccept(HWND hwnd, int socket);
void ProxyHandleServer(HWND hwnd, char *buf);

extern int TibiaVersionFound;

//extern FILE *fp;
extern FILE * fpRecord;

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
extern gzFile fpPlay;
extern int bytesPlayed;
extern int playSpeed;
extern int playing;
extern int abortPlayThread;
extern unsigned int msPlayed;
extern unsigned int msTotal;
extern int fastForwarding;
extern int *playMarkers;
extern int playMarkersCnt;
extern int nextPacket;
extern int numPackets;
extern int curPacket;

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
extern HBRUSH brushBlack, brushBlue, brushLtBlue, brushGrey, brushYellow, brushWhite;
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
extern char customloginserver[64];
extern int loginserver_last;

void RecordFillServerBox(void);
void RecordData(unsigned char *buf, short len);
void RecordStart(void);
void RecordEnd(void);
void RecordDisconnect(void);
void DoSocketRecord(HWND hwnd, int wEvent, int wError, int sock);
void RecordAddMarker(void);
LRESULT CALLBACK RecordChooseServerProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
/* end record.c */
