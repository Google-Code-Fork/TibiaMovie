/*
 * memory.c: memory injection
 *
 * Copyright 2005
 * See the COPYING file for more information on licensing and use.
 *
 * This file contains all the memory injection related functions. Since Tibia
 * doesn't allow you to set your own login server IP/address, we have to
 * change Tibia's memory to point to localhost, and act as a proxy.
 *
 * PS: Anyone know of a good way of tapping into send/recv for a specific
 * process? I know it can be done, but I've forgotten how. :P This would mean
 * you wouldn't even need a proxy solution at all. And no, it wasn't a global
 * packet-sniffer like libpcap.
 */


#include <windows.h>
#include <stdio.h>
#include <psapi.h>
#include "zlib.h"
#include "tibiamovie.h"

int memoryActivated = 0;

struct memorySaveStruct {
    LPVOID *address;
    char buf[512];
    int buflen;
};

struct memoryVersionStruct {
    int version;
    LPVOID *address;
    char buf[512];
    int buflen;
};

struct memoryVersionStruct memoryVersion[] = {
    /* 7.26 */
    { 726, (LPVOID)0x0048da18, "tibia2.cipsoft.com", 18 },
    { 726, (LPVOID)0x0048da2c, "tibia1.cipsoft.com", 18 },
    { 726, (LPVOID)0x0048da40, "server2.tibia.com",  17 },
    { 726, (LPVOID)0x0048da54, "server.tibia.com",   16 },
    { 726, (LPVOID)0x0051de08, "server.tibia.com",   16 },
    { 726, (LPVOID)0x0051de78, "server2.tibia.com",  17 },
    { 726, (LPVOID)0x0051dee8, "tibia1.cipsoft.com", 18 },
    { 726, (LPVOID)0x0051df58, "tibia2.cipsoft.com", 18 },
    
    /* 7.27 */
    { 727, (LPVOID)0x0048db30, "tibia2.cipsoft.com", 18 },
    { 727, (LPVOID)0x0048db44, "tibia1.cipsoft.com", 18 },
    { 727, (LPVOID)0x0048db58, "server2.tibia.com",  17 },
    { 727, (LPVOID)0x0048db6c, "server.tibia.com",   16 },
    { 727, (LPVOID)0x0051f9c8, "server.tibia.com",   16 },
    { 727, (LPVOID)0x0051fa38, "server2.tibia.com",  17 },
    { 727, (LPVOID)0x0051faa8, "tibia1.cipsoft.com", 18 },
    { 727, (LPVOID)0x0051fb18, "tibia2.cipsoft.com", 18 },

    /* 7.3 */
    { 730, (LPVOID)0x00480394, "tibia2.cipsoft.com", 18 },
    { 730, (LPVOID)0x004803a8, "tibia1.cipsoft.com", 18 },
    { 730, (LPVOID)0x004803bc, "server.tibia.com",   16 },
    { 730, (LPVOID)0x004803d0, "server2.tibia.com",  17 },
    { 730, (LPVOID)0x0051ce10, "server.tibia.com",   16 },
    { 730, (LPVOID)0x0051ce80, "server2.tibia.com",  17 },
    { 730, (LPVOID)0x0051cef0, "tibia1.cipsoft.com", 18 },
    { 730, (LPVOID)0x0051cf60, "tibia2.cipsoft.com", 18 },

    /* 7.35 */
    { 735, (LPVOID)0x004893ec, "tibia2.cipsoft.com", 18 },
    { 735, (LPVOID)0x00489400, "tibia1.cipsoft.com", 18 },
    { 735, (LPVOID)0x00489414, "server.tibia.com",   16 },
    { 735, (LPVOID)0x00489428, "server2.tibia.com",  17 },
    { 735, (LPVOID)0x005e8908, "server.tibia.com",   16 },
    { 735, (LPVOID)0x005e8978, "server2.tibia.com",  17 },
    { 735, (LPVOID)0x005e89e8, "tibia1.cipsoft.com", 18 },
    { 735, (LPVOID)0x005e8a58, "tibia2.cipsoft.com", 18 },

    /* 7.36 */
    { 736, (LPVOID)0x004893e4, "tibia2.cipsoft.com", 18 },
    { 736, (LPVOID)0x004893f8, "tibia1.cipsoft.com", 18 },
    { 736, (LPVOID)0x0048940c, "server.tibia.com",   16 },
    { 736, (LPVOID)0x00489420, "server2.tibia.com",  17 },
    { 736, (LPVOID)0x005e8908, "server.tibia.com",   16 },
    { 736, (LPVOID)0x005e8978, "server2.tibia.com",  17 },
    { 736, (LPVOID)0x005e89e8, "tibia1.cipsoft.com", 18 },
    { 736, (LPVOID)0x005e8a58, "tibia2.cipsoft.com", 18 },
    
    /* 7.4 */
    { 740, (LPVOID)0x004893ac, "tibia2.cipsoft.com", 18 },
    { 740, (LPVOID)0x004893c0, "tibia1.cipsoft.com", 18 },
    { 740, (LPVOID)0x004893d4, "server.tibia.com", 16 },
    { 740, (LPVOID)0x004893e8, "server2.tibia.com", 17 },
    { 740, (LPVOID)0x005e8908, "server.tibia.com", 16 },
    { 740, (LPVOID)0x005e8978, "server2.tibia.com", 17 },
    { 740, (LPVOID)0x005e89e8, "tibia1.cipsoft.com", 18 },
    { 740, (LPVOID)0x005e8a58, "tibia2.cipsoft.com", 18 },
    
/* add new (or old?) versions here :) */
   {   0, (LPVOID)0x00000000, "",                    0 }
};

struct memorySaveStruct memorySave[50];

int memorySaveCnt = 0;

HWND wTibia = NULL;
HINSTANCE hInstanceTibia = NULL;
int TibiaVersionFound = 0;

/* Get the Tibia Version from a specified Tibia.exe file */
int GetTibiaFileVersion(char *filename)
{
    DWORD size;
    LPVOID buf;
    LPTSTR block = "\\";
    VS_FIXEDFILEINFO *lpFFI;
    DWORD dwBufSize;
    char ver[64];
    
    size = GetFileVersionInfoSize(filename, NULL);
    
    if (!size) return 0;
    
    buf = (LPVOID)malloc(size);
    
    if (!buf) return 0;
    
    if (GetFileVersionInfo(filename, 0, size, buf)) {
        if (VerQueryValue(buf, block, (LPVOID *)&lpFFI, (UINT *)&dwBufSize)) {
            sprintf(ver, "%d%d%d", HIWORD(lpFFI->dwFileVersionMS),
                                   LOWORD(lpFFI->dwFileVersionMS),
                                   HIWORD(lpFFI->dwFileVersionLS)
            );
            
            free(buf);
            return atoi(ver);
        }
    }
    
    free(buf);
    return 0;
}

BOOL GetProcessFilenameFromProcess(HANDLE proc, char *out)
{
    if (GetModuleFileNameEx(proc, NULL, (LPTSTR)out, 1023)) {
        return 1;
    }
    
    return 0;
}

BOOL CALLBACK FindTibiaProc(HWND hwnd, LPARAM lparam)
{
    char buf[256];
    
    buf[0] = 0;
    
    GetWindowText(hwnd, buf, 255);
    
    if (!strncmp(buf, "Tibia   ", 8)) {
        wTibia = hwnd;
        hInstanceTibia = (HINSTANCE)GetWindowLong(wTibia, GWL_HINSTANCE);
        return FALSE;
    }
    
    wTibia = NULL;
    return TRUE;
}

int AdjustPrivileges(void) 
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;
    TOKEN_PRIVILEGES otp;
    DWORD dwSize = sizeof(TOKEN_PRIVILEGES);
    LUID luid;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
            return 1;
        return 0;
    }

    if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid)) {
        CloseHandle(hToken);
        return 0;
    }

    ZeroMemory(&tp, sizeof(tp));
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), &otp, &dwSize)) {
        CloseHandle(hToken);
        return 0;
    }

    CloseHandle(hToken);
    return 1;
}

/* only used for debugging */
char *MemoryProtectionType(int val)
{
    static char buf[512];
    
    buf[0] = '\0';
    
    switch (val) {
      case PAGE_EXECUTE:           strcpy(buf, "PAGE_EXECUTE");           break;
      case PAGE_EXECUTE_READ:      strcpy(buf, "PAGE_EXECUTE_READ");      break;
      case PAGE_EXECUTE_READWRITE: strcpy(buf, "PAGE_EXECUTE_READWRITE"); break;
      case PAGE_EXECUTE_WRITECOPY: strcpy(buf, "PAGE_EXECUTE_WRITECOPY"); break;
      case PAGE_NOACCESS:          strcpy(buf, "PAGE_NOACCESS");          break;
      case PAGE_READONLY:          strcpy(buf, "PAGE_READONLY");          break;
      case PAGE_READWRITE:         strcpy(buf, "PAGE_READWRITE");         break;
      case PAGE_WRITECOPY:         strcpy(buf, "PAGE_WRITECOPY");         break;
      default:                     strcpy(buf, "UNKNOWN");                break;
    }
    
    return buf;
}

/* currently this function is only used temporarily when tibia releases a new version
 * to search through the memory and dump the results. it COULD be a user option to
 * automatically search the memory as a last case effort, the reason it's not default
 * is searching all the memory takes a long time on older systems.
 */
int searchfound = 0;

void SearchPage(HANDLE hProcess, LPVOID start, int length)
{
    char buf[512];
    LPVOID scan;
    char *cnt;
    char *limit;
    LPVOID location;
    int locationlen;
    SIZE_T writelen;
    scan = (LPVOID)malloc(length);
    limit = (char *)scan + length;
        
    ReadProcessMemory(hProcess, start, scan, length, NULL);
    
    for (cnt = (char *)scan; cnt < limit; cnt++) {
        location = NULL;
        locationlen = 0;
        
        if (memcmp(cnt, "server.tibia.com", 16) == 0) {
            location = (LPVOID)((char *)start + (cnt - (char *)scan));
            locationlen = 16;
        }
        else if (memcmp(cnt, "server2.tibia.com", 17) == 0) {
            location = (LPVOID)((char *)start + (cnt - (char *)scan));
            locationlen = 17;
        }
        else if (memcmp(cnt, "tibia1.cipsoft.com", 17) == 0) {
            location = (LPVOID)((char *)start + (cnt - (char *)scan));
            locationlen = 18;
        }
        else if (memcmp(cnt, "tibia2.cipsoft.com", 18) == 0) {
            location = (LPVOID)((char *)start + (cnt - (char *)scan));
            locationlen = 18;
        }
        
        if (locationlen != 0) {
            ReadProcessMemory(hProcess, location, buf, locationlen, NULL);
            
            memorySave[memorySaveCnt].address = location;
            memcpy(memorySave[memorySaveCnt].buf, buf, locationlen);
            memorySave[memorySaveCnt].buflen = locationlen;
            memorySaveCnt++;
/*            searchfound = 1;*/
            
/* used when tibia releases new versions, easy copy/paste :P
            {
                FILE *fp = fopen("c:/memory.txt", "a");
                fprintf(fp, "{ (LPVOID)732, 0x%08x, \"%s\", %d },\n", (int)location, buf, locationlen);
                fclose(fp);
            }
*/            
            ZeroMemory(buf, 512);
            strcpy(buf, "127.0.0.1");
            WriteProcessMemory(hProcess, location, buf, locationlen, &writelen);
        }
    }
    
    free(scan);
}

void MemoryInjectionSearch(int toggle)
{
    DWORD dwProcessID;
    HANDLE hProcess;
    MEMORY_BASIC_INFORMATION meminfo;
    LPVOID cnt;
    int i;
    HWND wOldTibia = NULL;

    if (wTibia)
        wOldTibia = wTibia;

    EnumWindows(FindTibiaProc, 0);

    if (!wTibia || (wOldTibia && wOldTibia != wTibia)) {
        memoryActivated = 0;
        return;
    }

    GetWindowThreadProcessId(wTibia, &dwProcessID);
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessID);

    cnt = (LPVOID)0;

    if (toggle == 1) {
/*        FILE *fp=fopen("memorystruct.txt", "w");*/
        ZeroMemory(memorySave, sizeof(memorySave));
        memorySaveCnt = 0;
        
        while (VirtualQueryEx(hProcess, cnt, &meminfo, sizeof(meminfo)) > 0) {
            /* this seems to work, only check pages that are lower than 10mb, saves time */
            if (meminfo.State == MEM_COMMIT && (int)meminfo.RegionSize < 10000000 && (int)meminfo.AllocationBase == 0x00400000) {
/*                fprintf(fp, "baseaddress[%08x] allocationbase[%08x] allocationprotect[%d] regionsize[%d] state[%d] protect[%d] type[%d]\n",
                     meminfo.BaseAddress, meminfo.AllocationBase, meminfo.AllocationProtect, meminfo.RegionSize, meminfo.State, meminfo.Protect, meminfo.Type);
*/                
                SearchPage(hProcess, meminfo.BaseAddress, (int)meminfo.RegionSize);
                
/*                if (searchfound) {
                    fprintf(fp, "found\n");
                    searchfound = 0;
                }
*/
            }

            cnt = (LPVOID)((DWORD)meminfo.BaseAddress + (DWORD)meminfo.RegionSize);
        }
        
        memoryActivated = 1;
    }
    else if (memorySave[0].address != 0) {
        char buf[512];
        
        for (i = 0; memorySave[i].address != 0; i++) {
            ZeroMemory(buf, sizeof(buf));
            ReadProcessMemory(hProcess, memorySave[i].address, buf, memorySave[i].buflen, NULL);
            
            if (memcmp(buf, memorySave[i].buf, memorySave[i].buflen) == 0
                    || memcmp(buf, "127.0.0.1", 9) == 0) {
                WriteProcessMemory(hProcess, memorySave[i].address, memorySave[i].buf, memorySave[i].buflen, NULL);
            }
        }

        memoryActivated = 0;
        ZeroMemory(memorySave, sizeof(memorySave));
        memorySaveCnt = 0;
    }

    CloseHandle(hProcess);
    return;
}


void MemoryInjection(int toggle)
{
    DWORD dwProcessID;
    HANDLE hProcess;
    LPVOID cnt;
    HWND wOldTibia = NULL;
    int i;

    if (wTibia)
        wOldTibia = wTibia;

    EnumWindows(FindTibiaProc, 0);

    if (!wTibia || (wOldTibia && wOldTibia != wTibia)) {
        memoryActivated = 0;
        return;
    }

    GetWindowThreadProcessId(wTibia, &dwProcessID);
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessID);

    cnt = (LPVOID)0;

    if (toggle == 1) {
        char buf[512];
        TibiaVersionFound = 0;
        
        for (i = 0; memoryVersion[i].version != 0; i++) {
            ReadProcessMemory(hProcess, memoryVersion[i].address, buf, memoryVersion[i].buflen, NULL);

            if (memcmp(buf, memoryVersion[i].buf, memoryVersion[i].buflen) == 0
                || memcmp(buf, "127.0.0.1", 9) == 0) {
                TibiaVersionFound = memoryVersion[i].version;
            }
        }

        ZeroMemory(buf, sizeof(buf));
        strcpy(buf, "127.0.0.1");

        if (TibiaVersionFound != 0) {
            /* All address settings are expected values, go ahead and change them */
            for (i = 0; memoryVersion[i].version != 0; i++) {
                if (TibiaVersionFound != memoryVersion[i].version)
                    continue;

                WriteProcessMemory(hProcess, memoryVersion[i].address, buf, memoryVersion[i].buflen, NULL);
            }

            memoryActivated = 1;
        }
        else {
            char buf[1024];
            
            MemoryInjectionSearch(toggle);
            
            if (GetProcessFilenameFromProcess(hProcess, buf)) {
                TibiaVersionFound = GetTibiaFileVersion(buf);
            }
            else {
                TibiaVersionFound = 0;
            }
        }
    }
    else if (memorySave[0].address != 0) {
        MemoryInjectionSearch(toggle);
    }
    else if (TibiaVersionFound != 0) {
            char buf[512];

            for (i = 0; memoryVersion[i].version != 0; i++) {
                if (TibiaVersionFound != memoryVersion[i].version)
                    continue;

                ZeroMemory(buf, sizeof(buf));
                ReadProcessMemory(hProcess, memoryVersion[i].address, buf, memoryVersion[i].buflen, NULL);

                if (memcmp(buf, memoryVersion[i].buf, memoryVersion[i].buflen) == 0
                    || memcmp(buf, "127.0.0.1", 9) == 0) {
                    WriteProcessMemory(hProcess, memoryVersion[i].address, memoryVersion[i].buf, memoryVersion[i].buflen, NULL);
                }
            }
            memoryActivated = 0;
    }

    CloseHandle(hProcess);
    return;
}

