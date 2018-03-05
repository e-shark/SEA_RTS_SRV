#ifndef _OSWR_H
#define _OSWR_H
#include <fcntl.h>

#include "os_defs.h"
/*
 * OS: win/nix
 *
 *      Library of cross-platform wrappers for OS function calls
 */

 //---------------------Definitions for nix
#if defined _LINUX_
        //----------------Special function definitions
char* strupr(char *s);
char* strlwr(char *s);
int trySingletonLock(char *name);
int singletonUnlock(char *name);
int isSingletonRunning(char *name);
        //----------------Socket definitions
#include<errno.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/un.h>
#include<netinet/in.h>
#include<arpa/inet.h>
typedef int 	 SOCKET;
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR    (-1)
#define WSAEISCONN      EISCONN
#define SD_RECEIVE      SHUT_RD
#define SD_SEND         SHUT_WR
#define SD_BOTH         SHUT_RDWR
#define WSAGetLastError()       errno
#define closesocket             close
//---------------------Definitions for Windows
#else
        //----------------Special function definitions
#define pthread_mutex_t void*

#ifndef getpid
#define getpid GetCurrentProcessId
#endif
        //----------------Socket definitions
#define	MSG_NOSIGNAL	0
typedef int socklen_t;
#include<winsock2.h>
#endif
//------------------------------------------------------------------------------
void osSleep(DWORD);
DWORD osGetTickCount(VOID);

void*osInitKeyboard();
void osRestoreKeyboard();
int osGetKey(void*hConsole);

int osCreateFile(char*fname,int attr);
int osOpenFile(char*fname,int oflags);
int osReadFile(int handle, void *buf, unsigned len);
int osWriteFile(int handle, void *buf, unsigned len);
long osGetFileSize(int handle);
int osCloseFile(int handle);
int osTrancateFile(int handle,long size);

void*osLoadLibrary(char*LibName);
void*osGetProcAddress(void*hLibModule,char*procname);
BOOL osFreeLibrary(void*hLibModule);

void*osCreateSemaphore(char*semname,long initcount,long maxcount);
void*osOpenSemaphore(char*semname);
BOOL osWaitSemaphore(void*hSem,unsigned ms);
BOOL osReleaseSemaphore(void*hSem,long relcount);
BOOL osCloseSemaphore(void*hSem);

unsigned char*osCreateMmf(char*szMmfName,DWORD MmfSz,BOOL *MmfExists,void**hMmf);
unsigned char*osOpenMmf(char*szMmfName,void**phMmfRet);
void osCloseMmf(void*pMmf,void**hMmf);

void*osLaunchThread(void *handler,void*par);
BOOL osTerminateThread(void *thread,DWORD exitcode);
BOOL osWaitThread(void *thread, DWORD ms);

BOOL osInitTCP();
BOOL osFreeTCP();

BOOL osExec(char*cmd);
BOOL osExecute(char*cmd);
BOOL osShutDown(void);
BOOL SetProcessPrivilege(char*privname,BOOL enabled);

int osInitThreadMutex(pthread_mutex_t*mutex);
int osLockThreadMutex(pthread_mutex_t*mutex);
int osTryLockThreadMutex(pthread_mutex_t*mutex);
int osUnlockThreadMutex(pthread_mutex_t*mutex);
int osDestroyThreadMutex(pthread_mutex_t*mutex);

/*---170613, Named/unnamed mutexes functions.Now Windows version only released*/
int osCreateMutex( void**mutex, char* mutexname, bool lock = false );
int osDestroyMutex( void* hmutex );
int osLockMutex( void* hmutex, int tout=INFINITE );
int osUnlockMutex(void* hmutex );


BOOL osIsBadWritePtr(void*ptr,unsigned int sz);
BOOL osIsBadReadPtr(void*ptr,unsigned int sz);

void* osCreateNamedPipe( char* pipename );
void* osAcceptNamedPipe( void* hpipe );
void* osAcceptNamedPipeEx( void* hpipe, long timeout );
void* osConnectNamedPipe( char* pipename );
BOOL osIsNamedPipeConnected( void* hcon );
int osReadNamedPipe( void* hcon, unsigned char* buf, DWORD bufsize, BOOL isserver );
int osWriteNamedPipe( void* hcon, unsigned char* buf, DWORD bytestowtite, BOOL isserver );
void osDisconnectNamedPipeClient( void* hcon );
void osCloseNamedPipe( void* hpipe, char* pipename, BOOL isserver );

unsigned osGetProcVMSize();     // 05.09.16, vpr

#endif
