#define _CRT_SECURE_NO_WARNINGS
#include<winsock2.h>
#include<windows.h>
#include<string.h>
#include<io.h>
#include<sys/timeb.h>
#pragma hdrstop
#include<Psapi.h>
#pragma comment(lib, "Ws2_32.lib")
#include "oswr.h"
/*
 * OS: win
 *
 *      Library of cross-platform wrappers for OS function calls
 *      Windows implementation
 */


 /*******************************************************************************
 *      osSleep()
 *      Parameters:
 *              ms - пауза в msec.
 *      Remarks:
 *              Пауза процесса
 *      Returns:
 *              function has no return value
 ******************************************************************************/
void osSleep(DWORD ms){Sleep(ms);}
/*******************************************************************************
 *      osGetTickCount()
 *      Returns:
 *              function returns the number of milliseconds elapsed since OS was started
 ******************************************************************************/
DWORD osGetTickCount(VOID){return GetTickCount();}
/*------------------------------------------------------------------------------
 *	osInitKeyboard()
 *      osRestoreKeyboard()
 *----------------------------------------------------------------------------*/
void*osInitKeyboard(){return GetStdHandle(STD_INPUT_HANDLE);}
void osRestoreKeyboard(){}
/*------------------------------------------------------------------------------
 *	osGetKey() - return a key with no wait
 *----------------------------------------------------------------------------*/
int osGetKey(void*hConsole)
{
DWORD i;
INPUT_RECORD pb;

if(WaitForSingleObject(hConsole,10)!=WAIT_OBJECT_0)			return 0;
if(!ReadConsoleInput(hConsole,&pb,1,&i))				return 0;
if(!i)									return 0;
if( !((pb.EventType==KEY_EVENT) && (pb.Event.KeyEvent.bKeyDown)) )	return 0;
return pb.Event.KeyEvent.uChar.AsciiChar;
}
/*------------------------------------------------------------------------------
 *	osCreateFile()
 *	osOpenFile()
 *	osReadFile()
 *      osWriteFile()
 *	osGetFileSize()
 *	osCloseFile()
 *      osTrancateFile()
 *----------------------------------------------------------------------------*/
int osCreateFile(char*fname, int attr) { return /*_rtl*/_creat(fname, attr); }
int osOpenFile(char*fname, int oflags) { return /*_rtl*/_open(fname, oflags); }
int osReadFile(int handle, void *buf, unsigned len) { return /*_rtl*/_read(handle, buf, len); }
int osWriteFile(int handle, void *buf, unsigned len) { return /*_rtl*/_write(handle, buf, len); }
long osGetFileSize(int handle) { return filelength(handle); }
int osCloseFile(int handle) { return /*_rtl*/_close(handle); }
int osTrancateFile(int handle, long size) { return chsize(handle, size); }
/*******************************************************************************
 *      osLoadLibrary()
 *      osGetProcAddress()
 *      osFreeLibrary()
 *      Parameters:
 *      Remarks:
 *      Returns:
 *              function return TRUE in success, FALSE otherwice
 ******************************************************************************/
void*osLoadLibrary(char*LibName){        return LoadLibrary(LibName);}
void*osGetProcAddress(void*hLibModule,char*procname){return GetProcAddress((HMODULE)hLibModule,procname);}
BOOL osFreeLibrary(void*hLibModule){    return FreeLibrary((HMODULE)hLibModule);}
/*******************************************************************************
        osCreateSemaphore()
        osOpenSemaphore()
        osWaitSemaphore()
        osReleaseSemaphore()
        osCloseSemaphore()
 ******************************************************************************/
void* osCreateSemaphore( char* semname, long initcount, long maxcount ) {
SECURITY_DESCRIPTOR SecDesc;
SECURITY_ATTRIBUTES SecAttr;
        InitializeSecurityDescriptor(&SecDesc,SECURITY_DESCRIPTOR_REVISION );
        SetSecurityDescriptorDacl(&SecDesc,TRUE,(PACL)NULL,FALSE );
        SecAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        SecAttr.lpSecurityDescriptor = &SecDesc;
        SecAttr.bInheritHandle = FALSE;
return CreateSemaphore( &SecAttr, initcount, maxcount, semname );
}
void* osOpenSemaphore(char*semname) { return OpenSemaphore( SEMAPHORE_ALL_ACCESS, TRUE, semname ); }
BOOL osWaitSemaphore( void* hSem, unsigned ms ) {
        switch( WaitForSingleObject( hSem,ms ) ) {
                case WAIT_OBJECT_0:     return TRUE;
                case WAIT_ABANDONED:    return TRUE;
                default:                return FALSE;
        }
}
BOOL osReleaseSemaphore( void* hSem, long relcount ) { if( ReleaseSemaphore( hSem, relcount, NULL ) ) return TRUE; return FALSE; }
BOOL osCloseSemaphore( void* hSem ) { if( CloseHandle( hSem ) ) return TRUE; return FALSE; }
/*------------------------------------------------------------------------------
 *
 *	CreateMmf()
 *	-----------
 *	Создает memory mapped file с именем szMmfName размером MmfSz.
 *	Режим доступа к файлу-чтение/запись.Аттрибуты защиты-по умолчанию.
 *	Память под Файл выделяется из страничного системного файла.
 *	В случае, если файл существует, он открывается.
 *	Возвращает: указатель на начало оконного представления файла
 *	В случае, если файл существует, устанавливает флаг mmfexists в TRUE.
 *----------------------------------------------------------------------------*/
unsigned char*osCreateMmf(char*szMmfName,DWORD MmfSz,BOOL*MmfExists,void**phMmfRet)
{
HANDLE hMmf;
unsigned char*pMmf;
*MmfExists=FALSE;
SECURITY_DESCRIPTOR SecDesc;
SECURITY_ATTRIBUTES SecAttr;
//--------------Инициализация DAKL,позволяющего доступ к объекту любого процесса
InitializeSecurityDescriptor(&SecDesc,SECURITY_DESCRIPTOR_REVISION );
SetSecurityDescriptorDacl(&SecDesc,TRUE,(PACL)NULL,FALSE );
SecAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
SecAttr.lpSecurityDescriptor = &SecDesc;
SecAttr.bInheritHandle = FALSE;

hMmf=CreateFileMapping((HANDLE)0xffffffff,
			&SecAttr,
                        PAGE_READWRITE,
                        0,MmfSz	,
                        szMmfName);
if(hMmf==NULL)return NULL;
else if( GetLastError() == ERROR_ALREADY_EXISTS)*MmfExists=TRUE;
pMmf=(PBYTE)MapViewOfFile(hMmf,FILE_MAP_WRITE,0,0,0);
if(pMmf==NULL){CloseHandle(hMmf);hMmf=NULL;}
//!!!!CloseHandle(hMmf);//Это требует проверки-Проверено-Не работает!!!
*phMmfRet=hMmf;
return(pMmf);
}
/*----------------------------------------------------------------------------*/
unsigned char*osOpenMmf(char*szMmfName,void**phMmfRet)
{
HANDLE hMmf;
unsigned char*pMmf;

hMmf=OpenFileMapping(FILE_MAP_WRITE,0,szMmfName);
if(hMmf==NULL)return NULL;
pMmf=(PBYTE)MapViewOfFile(hMmf,FILE_MAP_WRITE,0,0,0);
if(pMmf==NULL){CloseHandle(hMmf);hMmf=NULL;}
*phMmfRet=hMmf;
return(pMmf);
}
/*------------------------------------------------------------------------------
 *
 *	CloseMmf()
 *
 *----------------------------------------------------------------------------*/
void osCloseMmf(void*pMmf,void**phMmf)
{
HANDLE hMmf;
        hMmf=*phMmf;
    	if(pMmf){ UnmapViewOfFile(pMmf); }
	if(hMmf){ CloseHandle(hMmf); *phMmf=0; }
 }
/*******************************************************************************
 *      osLaunchThread()
 *      Parameters:
 *              handler - процедура нити.
 *              par - параметр процедуры
 *      Remarks:
 *              Запуск потока исполнения
 *      Returns:
 *              function returns thread handler
 ******************************************************************************/
void*osLaunchThread(void *handler,void*par)
{
DWORD dwThreadId;
HANDLE h;
        if(handler==0)return 0;
	h=CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)handler,par,0,&dwThreadId);
return h;
}
/*******************************************************************************
 *      osTerminateThread()
 *      Parameters:
 *              thread - handler нити.
 *              exitcode - код возврата нити.
 *      Remarks:
 *              Завершение нити
 *      Returns:
 *              function returns TRUE in success, FALSE otherwise
 ******************************************************************************/
BOOL osTerminateThread(void *thread,DWORD exitcode)
{
BOOL retcode;
        retcode=TerminateThread(thread,exitcode);
        if(retcode)CloseHandle(thread);
        return  retcode;
}
/*******************************************************************************
 *      osWaitThread()
 *      Parameters:
 *              thread - handler нити.
 *              ms - время ожидания завершения нити
 *      Remarks:
 *              Ожидает завершения нити
 *      Returns:
 *              function returns TRUE in success, FALSE otherwise
 ******************************************************************************/
BOOL osWaitThread(void *thread, DWORD ms)
{
DWORD dwReturn;
        dwReturn = WaitForSingleObject(thread,ms);
        if ((WAIT_ABANDONED == dwReturn) ||	(WAIT_TIMEOUT == dwReturn))return FALSE;
        CloseHandle(thread);
return TRUE;
}
/*******************************************************************************
        osInitTCP()
        osFreeTCP()
 ******************************************************************************/
BOOL osInitTCP()
{
WSADATA wsaData;
//---Инициализация WSA Windows, запрос спецификации 1.1
if(WSAStartup(MAKEWORD(1, 1), &wsaData))        return FALSE;
if ( LOBYTE( wsaData.wVersion ) != 1 || HIBYTE( wsaData.wVersion ) != 1 )
{
        WSACleanup();
        return FALSE;
}
return TRUE;
}
BOOL osFreeTCP(){return (!WSACleanup());}
/*******************************************************************************
 *      osExec()
 *      Parameters:
 *              cmd - имя приложения и командная строка (д.б.отделена от имени пробелами)
 *      Remarks:
 *              Запуск внешнего модуля из текущей директории, если другая директория не указана явно
 *      Returns:
 *              TRUE - if success
 *              FALSE - on error
 ******************************************************************************/
BOOL osExec(char*cmd)
{
char cmdline[1000];
STARTUPINFO si;
PROCESS_INFORMATION pi;

        GetStartupInfo(&si);
        if(!strstr(cmd,"\\"))
        {
                GetCurrentDirectory(sizeof(cmdline),cmdline);
                strcat(cmdline,"\\");
                strcat(cmdline,cmd);
        }
        else strcpy(cmdline,cmd);
        return CreateProcess(NULL,cmdline,NULL,NULL,TRUE,NORMAL_PRIORITY_CLASS,NULL,NULL,&si,&pi);
}
/*******************************************************************************
*      osExecute()
 *      Remarks:
 *              Запуск внешнего модуля из текущей директории, если другая директория не указана явно
 *              Ожидает завершения дочрнего процесса
 *      Returns:
 *              TRUE - if success
 *              FALSE - on error
 ********************************************************************************/
BOOL osExecute(char*cmd)
{
    bool res;
    char cmdline[1000];
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    // формируем командную строку
    GetStartupInfo(&si);
    if (!strstr(cmd, "\\"))
    {
        GetCurrentDirectory(sizeof(cmdline), cmdline);
        strcat(cmdline, "\\");
        strcat(cmdline, cmd);
    }
    else strcpy(cmdline, cmd);

    // запускаем процесс
    res = CreateProcess(NULL, cmdline, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);

    // ждем окончания процесса
    if (res) {
        res = false;
        DWORD dwWait = WaitForSingleObject(pi.hProcess, INFINITE);
        if (dwWait == WAIT_OBJECT_0)
        {
            // программа благополучно завершилась
            res = true;

        }
        else if (dwWait == WAIT_ABANDONED)
        {
            // программа была насильно "прибита"

        }
        //  else ну и может быть другие варианты ожидания
    }

    return res;
}

/*******************************************************************************
 *      Перезагружает ВМ
 *
 ******************************************************************************/
BOOL osShutDown(void)
{
return osExec("reset.bat");
/*
        if(!SetProcessPrivilege(SE_SHUTDOWN_NAME,TRUE))return FALSE;
        ExitWindowsEx(EWX_FORCE|EWX_POWEROFF,0);
return TRUE;*/
}
/*******************************************************************************
 *      Устанавливает/снимает привилегию процесса
 *
 ******************************************************************************/
BOOL SetProcessPrivilege(char*privname,BOOL enabled)
{
HANDLE proctoken;
TOKEN_PRIVILEGES tp;

if(!OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,&proctoken))return FALSE;
tp.PrivilegeCount=1;
if(enabled)     tp.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;
else            tp.Privileges[0].Attributes=0;
if(!LookupPrivilegeValue(NULL,privname,&tp.Privileges[0].Luid)) return FALSE;
if(!AdjustTokenPrivileges(proctoken,FALSE,&tp,NULL,NULL,NULL))  return FALSE;
return TRUE;
}
/*******************************************************************************
 *      C-оболочки posix-вызовов
 *	pthread_mutex_init()
 *	pthread_mutex_lock()
 *	pthread_mutex_trylock()
 *	pthread_mutex_unlock()
 *	pthread_mutex_destroy()
 *	Обеспечивают синхронизацию объектов доступа различными нитями одного процесса
 *	Returns:	0-if succeess, error number otherwise
 ******************************************************************************/
int osInitThreadMutex(pthread_mutex_t*mutex)
{
HANDLE fmutex;
	fmutex=CreateMutex(NULL,FALSE,NULL);
        if(fmutex==NULL)return 1;
        *mutex=fmutex;
return 0;
}
int osLockThreadMutex(pthread_mutex_t*mutex)
{
	if(WAIT_OBJECT_0!=WaitForSingleObject(*mutex,INFINITE))return 1;
return 0;
}
int osTryLockThreadMutex(pthread_mutex_t*mutex)
{
	if(WAIT_OBJECT_0!=WaitForSingleObject(*mutex,0))return 1;
return 0;
}
int osUnlockThreadMutex(pthread_mutex_t*mutex)
{
	if(ReleaseMutex(*mutex))return 0;
return 1;
}
int osDestroyThreadMutex(pthread_mutex_t*mutex)
{
        if(CloseHandle(*mutex))return 0;
return 1;
}
/*******************************************************************************
vpr, 170613






                Library for Named(unnamed) Mutexes







*******************************************************************************/

/*******************************************************************************
 * Parameters:
 *      mutex - IN, pointer to memory location of mutex handle, if NULL, unnamed mutex will be created
 *      mutexname - IN, pointer to c-string with mutex name
 *      lock - IN, if true get ownership for newly created mutex
 * Remarks:
 *      Creates named mutex object
 * Returns: 1 on success, -1 on error, 0 if mutex already exists
 *
 */
int osCreateMutex( void**mutex, char*mutexname, bool lock )
{
//--------------Create DAKL for access by any process
SECURITY_DESCRIPTOR SecDesc;
SECURITY_ATTRIBUTES SecAttr;
InitializeSecurityDescriptor(&SecDesc,SECURITY_DESCRIPTOR_REVISION );
SetSecurityDescriptorDacl(&SecDesc,TRUE,(PACL)NULL,FALSE );
SecAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
SecAttr.lpSecurityDescriptor = &SecDesc;
SecAttr.bInheritHandle = FALSE;

	if( NULL == ( *mutex = CreateMutex( &SecAttr, lock, mutexname ) ) ) return -1;
	if( GetLastError() == ERROR_ALREADY_EXISTS )    return 0;
return 1;
}
/*******************************************************************************
 * Parameters:  hmutex - IN, mutex handle
 * Remarks:     Destroys mutex object
 * Returns:     1 on success, 0 otherwise
 */
int osDestroyMutex( void* hmutex ) { if( CloseHandle( hmutex ) ) return 1; return 0; }
/*******************************************************************************
 * Parameters:
 *      mutex - IN,mutex handle
 *      tout - IN, timeout during which the function waits the mutex to lock it if it is locked by another thread
 * Remarks:
 *      If the mutex has not been owned by any thread when function is called (mutex is in signaled state):
 *       - Gets the ownership of the mutex object (Sets the mutex to nonsignaled state) and returns
 *      If the mutex has been owned by some thread when function is called (mutex is in nonsignaled state):
 *      - waits for tout msec when the mutex will be released by owner thread, if so,
 *              - Gets the ownership of the mutex object (Sets the mutex to nonsignaled state) and returns
 *      - otherwise returns 0 value
 * Returns:
 *      1 on success, if mutex was locked by the function
 *      0 otherwise
 */
int osLockMutex( void* hmutex, int tout )
{
        switch( WaitForSingleObject( hmutex, tout ) ) {
                case WAIT_OBJECT_0:     return 1; // The mutex had no owner, got it
                case WAIT_ABANDONED:    return 1; // The thread that owned the mutex was terminated, got it
                case WAIT_TIMEOUT:      return 0; // wait interval elapsed
                default:                return 0; // error occured

        }
}
/*******************************************************************************
 * Parameters:  hmutex - IN, mutex handle
 * Remarks:     Release the owned mutex object (sets to signaled state):
 * Returns:     1 on success, 0 otherwise
 */
int osUnlockMutex( void* hmutex ) { if( ReleaseMutex( hmutex ) ) return 1; return 0; }





/*******************************************************************************
 ******************************************************************************/
BOOL osIsBadWritePtr( void* ptr, unsigned int sz) { return IsBadWritePtr( ptr, sz ); }
BOOL osIsBadReadPtr( void* ptr, unsigned int sz)  { return IsBadReadPtr( ptr, sz ); }
/*******************************************************************************
vpr, 160214






                Library for dealing with Named Pipes

        for Windows(as Named Pipes) & nix (as AF_UNIX sockets)






********************************************************************************

        Typical function invocation chain:
Server:
        hpipe = osCreateNamedPipe( pipename );
        hcon = osAcceptNamedPipe( hpipe );
        if(     ( osReadNamedPipe( hcon )  < 0 ) ||
                ( osWriteNamedPipe( hcon ) < 0 ) )
        {
                osDisconnectNamedPipeClient( hcon );
                osAcceptNamedPipe( hpipe );
        }
        osCloseNamedPipe( hpipe );
Client:
        hcon = osConnectNamedPipe( pipename );
        if(     ( osReadNamedPipe( hcon )  < 0 ) ||
                ( osWriteNamedPipe( hcon ) < 0 ) )
        {
                osCloseNamedPipe( hcon );
                osConnectNamedPipe( pipename );
        }
        osCloseNamedPipe( hcon );
*/
/*******************************************************************************
 *      osCreateNamedPipe()
 * Remarks:
 *      Creates nonblocking bi-directional named pipe for
 *      Server process to communicate
 *      with remote named pipe clients
 *      Windows:        in Message Mode !!!
 * Parameters:
 *      pipename - pointer to C-string with pipe name
 *      \\\\.\\pipe\\pipename - for Windows
 *      /tmp/pipename - for nix (or any other file name)
 * Returns:
 *      The Handle for opened named pipe or INVALID_HANDLE_VALUE if create fails
 */
void* osCreateNamedPipe( char* pipename )
{
HANDLE hpipe;
DWORD dwPipeMode = PIPE_NOWAIT | PIPE_TYPE_BYTE | PIPE_READMODE_BYTE;

        if( pipename == NULL )return INVALID_HANDLE_VALUE;
        hpipe = CreateNamedPipe( pipename, PIPE_ACCESS_DUPLEX, dwPipeMode, 1, 1024, 1024, 2000, NULL );
return hpipe;
}
/*******************************************************************************
 *      osAcceptNamedPipe()
 * Remarks:
 *      Accepts client connection for Server pipe
 *      Blocks the execution up to moment, when client gets connected
 * Parameters:
 *      hpipe   - the handle of the named pipe
 * Returns:
 *      The Handle of client connection or INVALID_HANDLE_VALUE if error occured
 */
void* osAcceptNamedPipe( void* hpipe )
{
        if( ( !hpipe ) || ( hpipe == INVALID_HANDLE_VALUE ) ) return INVALID_HANDLE_VALUE;
        for(;;)
        {
                if( ConnectNamedPipe( hpipe, NULL ) ) return hpipe;
                Sleep(100);
                switch( GetLastError() )
                {
                        case ERROR_PIPE_CONNECTED:      return hpipe; // OK, client connected
                        case ERROR_NO_DATA:
                                DisconnectNamedPipe( hpipe );// previous client closed its connection, have to reconnect
                        case ERROR_PIPE_LISTENING:      // no client is connected
                        case ERROR_IO_PENDING:          // ConnectNamedPipe in progress, return 0
                        default:break;
                }
        }
}
/*******************************************************************************
 *      osAcceptNamedPipeEx()
 * Remarks:
 *      Accepts client connection for Server pipe
 *      Blocks the execution up to moment, when client gets connected or timeout elapsed
 * Parameters:
 *      hpipe   - the handle of the named pipe
 * Returns:
 *      The Handle of client connection or INVALID_HANDLE_VALUE if error occured
 */
void* osAcceptNamedPipeEx( void* hpipe, long timeout )
{
struct timeb tb,tb1;

        if( ( !hpipe ) || ( hpipe == INVALID_HANDLE_VALUE ) ) return INVALID_HANDLE_VALUE;
        ftime( &tb1 );
        for(;;)
        {
                if( ConnectNamedPipe( hpipe, NULL ) ) return hpipe;
                switch( GetLastError() )
                {
                        case ERROR_PIPE_CONNECTED:      return hpipe; // OK, client connected
                        case ERROR_NO_DATA:
                                DisconnectNamedPipe( hpipe );// previous client closed its connection, have to reconnect
                        case ERROR_PIPE_LISTENING:      // no client is connected
                        case ERROR_IO_PENDING:          // ConnectNamedPipe in progress, return 0
                        default:break;
                }
                ftime( &tb );
                if( ( ( tb.time - tb1.time ) * 1000 + ( tb.millitm - tb1.millitm ) ) >= timeout )return INVALID_HANDLE_VALUE;
                Sleep(1);
        }
}
/*******************************************************************************
 *      osConnectNamedPipe()
 * Remarks:
 *      Opens the existed named pipe for communicate
 *      Client with remote named pipe server
 *      Linux:    Blocks execution for 20seconds while connect is running
 *      Windows:  Blocks execution for 5seconds
 * Parameters:
 *      pipename - pointer to C-string with pipe name
 * Returns:
 *      The Handle of named pipe connection or INVALID_HANDLE_VALUE if open fails
 */
void* osConnectNamedPipe( char* pipename )
{
HANDLE hpipe = INVALID_HANDLE_VALUE;

        if( pipename == NULL )return INVALID_HANDLE_VALUE;
        if( WaitNamedPipe( pipename, 5000) ) // use WaitNamedPipe() to prevent blocking of program execution  in CreateFile()
        {
                hpipe = CreateFile( pipename, GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
//                if( hpipe == INVALID_HANDLE_VALUE ){DWORD laster = GetLastError();}
        }
        return hpipe;
}
/*******************************************************************************
 * Remarks:
 *      Tests the pipe connection state
 * Parameters:
 *      hcon   - the handle of the named pipe connection
 * Returns:
 *      TRUE if connected, FALSE otherwise
 */
BOOL osIsNamedPipeConnected( void* hcon )
{
        if( hcon == INVALID_HANDLE_VALUE )     return FALSE;
        if( PeekNamedPipe( hcon, NULL, NULL, NULL, NULL, NULL ) ) return TRUE;
return FALSE;
}
/*******************************************************************************
 *      osReadNamedPipe()
 * Remarks:
 *      Reads from named pipe.
 *      NON BLOCKING
 * Parameters:
 *      hcon   - the handle of the named pipe connection
 *      buf     - the pointer to buffer for read bytes
 *      bufsize - size of the buf in bytes
 *      isserver - server mode flag
 * Returns:
 *      on success function returns number of bytes read
 *      on error:
 *      -1 - error - client disconnected
 *      -2 - error reading pipe
 */
int osReadNamedPipe( void* hcon, unsigned char* buf, DWORD bufsize, BOOL isserver )
{
DWORD bytesread = 0, TotalBytesAvail;
        if( hcon == INVALID_HANDLE_VALUE )      return 0;
        if( bufsize == 0)                       return 0;
        if( buf == NULL )                       return 0;
        //---Manage the connection in server mode
        if( isserver )
        {
                ConnectNamedPipe( hcon, NULL );
                switch( GetLastError() )
                {
                        case ERROR_PIPE_CONNECTED:      // OK, go to read
                                break;
                        case ERROR_NO_DATA:             // previous client closed its connection, have to reconnect
                                DisconnectNamedPipe( hcon );
                                return -1;
                        case ERROR_PIPE_LISTENING:      // no client is connected
                                return -1;
                        case ERROR_IO_PENDING:          // ConnectNamedPipe in progress, return 0
                        default:  return 0;
                }
        }
        if( PeekNamedPipe( hcon, buf, bufsize, &bytesread, &TotalBytesAvail, NULL) ) // use PeekNamedPipe() to prevent blocking of program execution  in ReadFile()
        {
                if( TotalBytesAvail )
                {
                        memset( buf, 0, bufsize ); // Prepare buffer for C-string
                        if( TotalBytesAvail > bufsize )TotalBytesAvail = bufsize;
                        if( !ReadFile( hcon, buf, TotalBytesAvail, &bytesread, NULL) ) return -2;
                }
        }
        else return -2;
return bytesread;
}
/*******************************************************************************
 *      osWriteNamedPipe()
 * Remarks:
 *      Writes to the named pipe.
 *      NON BLOCKING
 * Parameters:
 *      hcon   - the handle of the named pipe connection
 *      buf     - the pointer to buffer for read bytes
 *      bytestowtite - number of bytes in buf
 *      isserver - server mode flag
 * Returns:
 *      on success function returns number of bytes read
 *      on error:
 *      -1 - error client disconnected
 *      -2 - error writing pipe
 */
int osWriteNamedPipe( void* hcon, unsigned char* buf, DWORD bytestowtite, BOOL isserver )
{
DWORD byteswritten;
        if( hcon == INVALID_HANDLE_VALUE )      return 0;
        if( bytestowtite == 0)                  return 0;
        if( buf == NULL )                       return 0;
        //---Manage the connection in server mode
        if( isserver )
        {
                ConnectNamedPipe( hcon, NULL );
                switch( GetLastError() )
                {
                       case ERROR_PIPE_CONNECTED:break;
                        case ERROR_NO_DATA: DisconnectNamedPipe( hcon );return -1;
                        case ERROR_PIPE_LISTENING:return -1;
                        case ERROR_IO_PENDING:
                        default: return 0;
                }
        }
        if( !WriteFile( hcon, buf, bytestowtite, &byteswritten, NULL ) )return -2;
return byteswritten;
}
/*******************************************************************************
 *      osDisconnectNamedPipeClient()
 * Remarks:
 *      This function is intended for server to force disconnect the client from the server.
 * Parameters:
 *      hcon   - the handle of the named pipe
 */
void osDisconnectNamedPipeClient( void* hcon )
{
        DisconnectNamedPipe( hcon );
}
/*******************************************************************************
 *      osCloseNamedPipe()
 * Remarks:
 *      Closes the named pipe.
 * Parameters:
 *      hpipe   - the handle of the named pipe
 *      pipename - pointer to C-string with pipe name
 *      isserver - server mode flag
 */
void osCloseNamedPipe( void* hpipe, char* pipename, BOOL isserver )
{
        if( hpipe == INVALID_HANDLE_VALUE )     return;
        if( isserver ) DisconnectNamedPipe( hpipe );
        CloseHandle( hpipe );
}

/*******************************************************************************
 *      osGetProcVMSize()
 * Remarks:
 *      Returns the virtual memory size of a current process.
 */
unsigned osGetProcVMSize( )
{
/*
        PROCESS_MEMORY_COUNTERS pmc;
        GetProcessMemoryInfo( GetCurrentProcess(), &pmc, sizeof(pmc) );
        return pmc.WorkingSetSize + pmc.PagefileUsage;
*/
return 0;
}

