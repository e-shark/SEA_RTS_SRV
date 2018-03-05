#include<windows.h>
#include<stdio.h>
/* 160214, vpr
 *                      UART Library
 *
 * Here is a set of simple functions for dealing with UART asynchronously
 * under Windows operating system
 * An appropriate set for nix you will find in osUartX.cpp
 */

/*
        Under Windows OS the Library uses overlammed operations for reading & writing the UART.
        To perform this, the library uses OVERLAPPED structure, placed in global application memory,
        not in local function stack! It prevents errors in cases when read or write operation is still pending,
        when an osUartRead/Write function exits after timeout occurred.
        And static memory is used.
        Because the library is in C-language we don't use classes and can't define these structures as class members
        Because to do the dynamic memory allocation in C-functions we need to pass to user application the pointer,
        but we try to avoid extra parameters in library functions, and we're trying
        to use the native OS handle in it.
        But as payment for all these we get the limit for ports number, and memory overuse
        So, below are placed the mentioned objects.
 */
#define MAXUPORTS 128
static struct{
 HANDLE hport;                  // port Handle
 OVERLAPPED oread;              // structure for overlapped read
 OVERLAPPED owrite;             // structure for overlapped write
}hports[MAXUPORTS];             // Structure for performing an overlapped uarts read/write operations
static LONG uartsnumber;        // The number of opened UART ports
static HANDLE hportsmutex;      // The mutex for the hports access arbitration
/*******************************************************************************
 *      osUartSetMode()
 * Remarks:
 *      Sets the mode for the opened UART port.
 *      Switches the UART to the  raw binary mode!
 *      (Turns off input and output processing of all special characters, except XON/XOFF)!
 * Parameters:
 *      hport - the OS handle of opened uart port
 *      mode - pointer to C-string with port parameters, she should be formatted as follows:
 *
 *      "[BAUD=b] [PARITY=p] [DATA=d] [STOP=s] [to=on|off] [xon=on|off] [odsr=on|off] [octs=on|off]
 *      [dtr=on|off|hs] [rts=on|off|hs|tg] [idsr=on|off]"
 *      where:
 *  BAUD - port baud rate, b=110|300|600|1200|2400|4800|9600|14400|19200|\n\
 *                           38400|56000|57600|115200|128000|256000\n\
 *  PARITY - sets the port's parity, b=N|E(default)|O|M|S\n\
 *  DATA - sets the number of information bits, d=5|6|7(default)|8\n\
 *  STOP - sets the stop bits number, s=1(default)|2(default when 110)\n\
 *  to   - sets the port's infinite timeout processing to ONN|OFF(default)\n\
 *  xon  - XON/XOFF flow control \n\
 *  odsr - sets the port's transmitter behaviour depending on state of DSR input\n\
 *  octs - sets the port's transmitter behaviour depending on state of CTS input\n\
 *  dtr  - sets the DTR initial state to ON|OFF|HANDSHAKE\n\
 *  rts  - sets the RTS initial state to ON|OFF|HANDSHAKE|TOGGLE\n\
 *  idsr - sets the port's receiver behaviour depending on state of DSR input\n\
 *
 *      for example: "baud=9600 parity=N data=8 stop=1",
 * Returns: 1 on success, 0 on error
 */
int osUartSetMode( void* hport, char*mode )
{
DCB udcb;
COMMTIMEOUTS ctouts;
char*p;
unsigned i;
        if( (!hport) || (!mode) )               return 0;
        if( !GetCommState( hport, &udcb ) )     return 0;
        if( !GetCommTimeouts( hport, &ctouts ) )return 0;
        ctouts.ReadIntervalTimeout        = 10;//MAXDWORD;//
        ctouts.ReadTotalTimeoutMultiplier = 0;//MAXDWORD;//
        ctouts.ReadTotalTimeoutConstant   = 0; //100//
        ctouts.WriteTotalTimeoutMultiplier = 0;
        ctouts.WriteTotalTimeoutConstant = 0;

        strupr( mode );
        if( ( p = strstr( mode, "BAUD=" ) ) != NULL )if( sscanf( p+5, "%d", &i ) == 1 )udcb.BaudRate = i;
        if( ( p = strstr( mode, "PARITY=" ) ) != NULL )switch( *(p+7) )
        {
                case 'N': udcb.fParity = 0; udcb.Parity = NOPARITY; break;
                case 'O': udcb.fParity = 1; udcb.Parity = ODDPARITY; break;
                case 'E': udcb.fParity = 1; udcb.Parity = EVENPARITY; break;
                case 'M': udcb.fParity = 1; udcb.Parity = MARKPARITY; break;
                case 'S': udcb.fParity = 1; udcb.Parity = SPACEPARITY; break;
        }
        if( ( p = strstr( mode, "DATA=" ) ) != NULL )if( sscanf( p+5, "%d", &i ) == 1 )if((i>4)&&(i<9))udcb.ByteSize = i;
        if( ( p = strstr( mode, "STOP=" ) ) != NULL )if( sscanf( p+5, "%d", &i ) == 1 ){if(i==1)i=0;if(i<3)udcb.StopBits = i;}
        if( ( p = strstr( mode, "TO=" ) ) != NULL )
        {
                if( strncmp(p+3,"ON", 2) == 0 ){}
                if( strncmp(p+3,"OFF",3) == 0 ){ memset( &ctouts,0,sizeof( ctouts ) ); }
        }
        if( ( p = strstr( mode, "XON=" ) ) != NULL )
        {
                if( strncmp(p+4,"ON", 2) == 0 ){udcb.fOutX = 1;udcb.fInX = 1;}
                if( strncmp(p+4,"OFF",3) == 0 ){udcb.fOutX = 0;udcb.fInX = 0;}
        }
        if( ( p = strstr( mode, "ODSR=" ) ) != NULL )
        {
                if( strncmp(p+5,"ON", 2) == 0 ){udcb.fOutxDsrFlow = 1;}
                if( strncmp(p+5,"OFF",3) == 0 ){udcb.fOutxDsrFlow = 0;}
        }
        if( ( p = strstr( mode, "OCTS=" ) ) != NULL )
        {
                if( strncmp(p+5,"ON", 2) == 0 ){udcb.fOutxCtsFlow = 1;}
                if( strncmp(p+5,"OFF",3) == 0 ){udcb.fOutxCtsFlow = 0;}
        }
        if( ( p = strstr( mode, "DTR=" ) ) != NULL )
        {
                if( strncmp(p+4,"ON", 2) == 0 ){udcb.fDtrControl = DTR_CONTROL_ENABLE;}
                if( strncmp(p+4,"OFF",3) == 0 ){udcb.fDtrControl = DTR_CONTROL_DISABLE;}
                if( strncmp(p+4,"HS", 2) == 0 ){udcb.fDtrControl = DTR_CONTROL_HANDSHAKE;}
        }
        if( ( p = strstr( mode, "RTS=" ) ) != NULL )
        {
                if( strncmp(p+4,"ON", 2) == 0 ){udcb.fRtsControl = RTS_CONTROL_ENABLE;}
                if( strncmp(p+4,"OFF",3) == 0 ){udcb.fRtsControl = RTS_CONTROL_DISABLE;}
                if( strncmp(p+4,"HS", 2) == 0 ){udcb.fRtsControl = RTS_CONTROL_HANDSHAKE;}
                if( strncmp(p+4,"TG", 2) == 0 ){udcb.fRtsControl = RTS_CONTROL_TOGGLE;}
        }
        if( ( p = strstr( mode, "IDSR=" ) ) != NULL )
        {
                if( strncmp(p+5,"ON", 2) == 0 ){udcb.fDsrSensitivity = 1;}
                if( strncmp(p+5,"OFF",3) == 0 ){udcb.fDsrSensitivity = 0;}
        }

        if( !SetCommTimeouts( hport, &ctouts ) )return 0;
        if( !SetCommState( hport, &udcb ) )     return 0;
        return 1;
}
/*******************************************************************************
 *      osUartOpen()
 * Remarks:
 *      Function opens UART port for asynchronous IO in raw binary mode (not a tty)
 * Parameters:
 *      portname - OS name for uart device, com1, or \\\\.\\com10 (Windows), or /dev/ttyS0 (Linux)
 *      mode - pointer to C-string with port parameters, see description above
 * Returns:
 *      OS handle for uart port, or
 *      INVALID_HANDLE_VALUE - if error occurred
 */
void* osUartOpen( char*portname, char*mode )
{
HANDLE hport;
int i;
        if( !portname )return INVALID_HANDLE_VALUE;
        hport = CreateFile( portname, GENERIC_READ | GENERIC_WRITE, 0, // share mode
          NULL,					// address of security descriptor
          OPEN_EXISTING,			// how to create
          FILE_FLAG_OVERLAPPED,			// file attributes
          NULL ); 				// handle of file with attributes to copy
        if( hport == INVALID_HANDLE_VALUE )return hport;

        PurgeComm( hport,PURGE_TXCLEAR|PURGE_RXCLEAR ); // clears the device driver input & output buffers
        if( FALSE == osUartSetMode( hport, mode ) ) { m1: CloseHandle( hport ); return INVALID_HANDLE_VALUE; }

        if( InterlockedExchangeAdd( &uartsnumber,1 ) == 0 ){ Sleep( 100 ); if( hportsmutex == NULL )hportsmutex = CreateMutex( NULL, FALSE, NULL ); }
        if( hportsmutex == NULL ) goto m1;
        WaitForSingleObject( hportsmutex, INFINITE );
        for( i = 0; i < MAXUPORTS; i++ )if( hports[ i ].hport == 0 )
        {
          hports[ i ].hport = hport;
          if( NULL == ( hports[ i ].oread.hEvent  = CreateEvent(NULL, TRUE, FALSE, NULL ) ) ) goto m1;
          if( NULL == ( hports[ i ].owrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL ) ) ) goto m1;
          break;
        }
        ReleaseMutex( hportsmutex );

return hport;
}
/*******************************************************************************
 *      osUartClose()
 * Parameters:
 *      hport - the OS handle of opened uart port
 * Returns:
 *      Function return 1 on success, 0 otherwise, to get error information
 *      GetLastError()(under Windows) or errno( under nix) should be used
 */
int osUartClose( void* hport )
{
int i;
        if( hport == INVALID_HANDLE_VALUE )     return 0;
        if( !hport )                            return 0;
        PurgeComm( hport,PURGE_TXABORT|PURGE_RXABORT ); // cancel all pending overlapped operations

        WaitForSingleObject( hportsmutex, INFINITE );
        for( i = 0; i < MAXUPORTS; i++ )if( hports[ i ].hport == hport )
        {
                CloseHandle( hports[ i ].oread.hEvent );
                CloseHandle( hports[ i ].owrite.hEvent );
                hports[ i ].hport = 0;
                break;
        }
        ReleaseMutex( hportsmutex );
        if( InterlockedExchangeAdd( &uartsnumber, -1 ) == 1 ){ CloseHandle( hportsmutex ); hportsmutex = 0; }

        return CloseHandle( hport );
}

/*******************************************************************************
 *      osUartRead()
 * Parameters:
 *      hport - the OS handle of opened uart port
 *      buf - pointer to the buffer for incoming bytes
 *      len - the length of buf in bytes
 *      tout - timeout for reading in msec
 * Returns:
 *      Function returns the number of bytes read, or
 *      -1 if error occurred
 */
int osUartRead( void* hport, unsigned char*buf, unsigned len, unsigned tout )
{
COMMTIMEOUTS ctouts;
DWORD laster, bytesread = 0;
OVERLAPPED* oread = 0;
int i;

  if( hport == INVALID_HANDLE_VALUE )   return -1;
  if( (!hport) || (!buf) || (!len) )    return 0;

  WaitForSingleObject( hportsmutex, INFINITE );
  for( i = 0; i < MAXUPORTS; i++ )if( hports[ i ].hport == hport ){ oread = &hports[ i ].oread; break; }
  ReleaseMutex( hportsmutex );
  if( !oread )return -1;

        /*--Version with rx bytes count check
COMSTAT cstat;
        for( unsigned etout = 0; etout <= tout; etout = etout+100 )
        {
                if( FALSE == ClearCommError( hport, &laster, &cstat ) ) return -1;
                if( cstat.cbInQue )
                {
                        if( FALSE == ReadFile( hport, buf, len, &bytesread, NULL ) )return -1;
                        break;
                }
                if( etout < tout )Sleep( 100 );
        }
*/
        /*--Version with commtimeouts
COMMTIMEOUTS ctouts;

        if( FALSE == GetCommTimeouts( hport, &ctouts ) )             return -1;
        ctouts.ReadIntervalTimeout        = MAXDWORD;
        ctouts.ReadTotalTimeoutMultiplier = MAXDWORD;
        ctouts.ReadTotalTimeoutConstant   = tout;
        if( FALSE == SetCommTimeouts( hport, &ctouts ) )             return -1;
        if( FALSE == ReadFile( hport, buf, len, &bytesread, NULL ) ) return -1;
*/
        /*--Overlapped version*/
  if( FALSE == GetCommTimeouts( hport, &ctouts ) )             return -1;
  ctouts.ReadIntervalTimeout        = MAXDWORD;
  ctouts.ReadTotalTimeoutMultiplier = MAXDWORD;
  ctouts.ReadTotalTimeoutConstant   = tout;
  if( FALSE == SetCommTimeouts( hport, &ctouts ) )             return -1;

  if( TRUE == ReadFile( hport, buf, len, &bytesread, oread ) ){goto m1;}
  if( ( laster = GetLastError() ) == NO_ERROR )goto m1;
  else if( laster != ERROR_IO_PENDING ){ m2:return -1; }
  switch ( WaitForSingleObject( oread->hEvent, tout+50 ) ) // !Here tout has to be greater than ReadTotalTimeoutConstant,
  {          // otherwise she loses bytes after trapping into WAIT_TIMEOUT case instead WAIT_OBJECT_0 (overlapped Read continues). Should be puzzled out!
        case WAIT_FAILED:goto m2;       // be sure port was closed, or not likely but error occurred
        case WAIT_OBJECT_0:
                if( FALSE == GetOverlappedResult( hport, oread, &bytesread, FALSE) )bytesread = 0;break;
        case WAIT_TIMEOUT:  break;// If we got here, then reading still pending, and if it bad, we should know how to cancel it
        default: break;
  }
m1:
return bytesread;
}

/*******************************************************************************
 *      osUartWrite()
 * Parameters:
 *      hport - the OS handle of opened uart port
 *      buf - pointer to the output buffer
 *      num - the number of bytes for output
 *      tout - timeout for writing in msec, if set to zero (default),
 *              calculated value will be used, according to uart baud rate
 *              and number of bytes to write
 * Returns:
 *      Function returns the number of written bytes, or
 *      -1 if error occurred
 */
int osUartWrite( void* hport, unsigned char*buf, unsigned num, unsigned tout )
{
DWORD laster, byteswritten = 0;
OVERLAPPED* owrite = 0;
int i;

  if( hport == INVALID_HANDLE_VALUE )   return -1;
  if( (!hport) || (!buf) || (!num) )    return 0;

  WaitForSingleObject( hportsmutex, INFINITE );
  for( i = 0; i < MAXUPORTS; i++ )if( hports[ i ].hport == hport ){ owrite = &hports[ i ].owrite; break; }
  ReleaseMutex( hportsmutex );
  if( !owrite )return -1;

        /*--Version with commtimeouts
COMMTIMEOUTS ctouts;
        if( FALSE == GetCommTimeouts( hport, &ctouts ) )                 return -1;
        ctouts.WriteTotalTimeoutMultiplier = 0;
        ctouts.WriteTotalTimeoutConstant = tout;
        if( FALSE == SetCommTimeouts( hport, &ctouts ) )                 return -1;
        if( FALSE == WriteFile( hport, buf, num, &byteswritten, NULL ) ) return -1;
*/
        /*--Overlapped version*/
//CancelIo (hport);
  if( WriteFile( hport, buf, num, &byteswritten, owrite) )     goto m1;
  if( ( laster = GetLastError() ) == NO_ERROR )                goto m1;
  else if( laster != ERROR_IO_PENDING ){ m2:return -1; }

  switch ( WaitForSingleObject( owrite->hEvent, tout ? tout : num*100 ) )
  {
        case WAIT_FAILED:goto m2;       // be sure port was closed, or not likely but error occurred
        case WAIT_OBJECT_0:
                if( FALSE == GetOverlappedResult( hport, owrite, &byteswritten, FALSE) ) byteswritten = 0;
                break;
        case WAIT_TIMEOUT:
        default: break;
  }
m1:
return byteswritten;
}
/*******************************************************************************
 *      osUartFlush()
 * Parameters:
 *      hport - the OS handle of opened uart port
 *      sel - what to flush, 1 - input buffer, 2 - output buffer, 3 - all
 * Returns:
 *      1 on success, 0 on error
 */
int osUartFlush( void* hport, int sel )
{
return PurgeComm( hport, ( ( sel & 1 ) ? PURGE_RXCLEAR : 0 ) | ( ( sel & 2 ) ? PURGE_TXCLEAR : 0 ) );
}
/*******************************************************************************
 *      osUartSetDTR()
 * Remarks:     Sets the DTR line
 * Parameters:  hport - the OS handle of opened uart port
 *              state - 1 - ON, 0 - OFF
 * Returns:     1 on success, 0 on error
 */
int osUartSetDTR( void* hport, int  state ){ return EscapeCommFunction( hport, state ? SETDTR : CLRDTR ); }
/*******************************************************************************
 *      osUartSetRTS()
 * Remarks:     Sets the RTS line
 * Parameters:  hport - the OS handle of opened uart port
 *              state - 1 - ON, 0 - OFF
 * Returns:     1 on success, 0 on error
 */
int osUartSetRTS( void* hport, int state ){ return EscapeCommFunction( hport, state ? SETRTS : CLRRTS ); }
/*******************************************************************************
 *      osUartSetBreak()
 * Remarks:     Sets the UART tx line in a break state for tout time interval or for infinite time if tout is 0,
 *              Function is blocking the execution until tout will elapse!
 * Parameters:  hport - the OS handle of opened uart port
 *              tout - timeout in msec
 * Returns:     1 on success, 0 on error
 */
int osUartSetBreak( void* hport, unsigned tout )
{
        if( !SetCommBreak( hport ) )    return 0;
        if( tout == 0 )                 return 1;
        Sleep( tout );
        if( !ClearCommBreak( hport ) )  return 0;
return 1;
}
/*******************************************************************************
 *      osUartResetBreak()
 * Remarks:     Switches the UART tx line to normal state
 * Parameters:  hport - the OS handle of opened uart port
 * Returns:     1 on success, 0 on error
 */
int osUartResetBreak( void* hport ){ return ClearCommBreak( hport ); }
/*******************************************************************************
 *      osUartGetCTS()
 * Remarks:     Gets the CTS line state
 * Parameters:  hport - the OS handle of opened uart port
 * Returns:     0/1 - CTS ON/OFF, -1 on error
 */
int osUartGetCTS( void* hport )
{
DWORD state;
        if( !GetCommModemStatus( hport, &state ) )return -1;
return ( state & MS_CTS_ON ) ? 1: 0;
}
/*******************************************************************************
 *      osUartGetDSR()
 * Remarks:     Gets the DSR line state
 * Parameters:  hport - the OS handle of opened uart port
 * Returns:     0/1 - DSR ON/OFF, -1 on error
 */
int osUartGetDSR( void* hport )
{
DWORD state;
        if( !GetCommModemStatus( hport, &state ) )return -1;
return ( state & MS_DSR_ON ) ? 1: 0;
}
/*******************************************************************************
 *      osUartGetDCD()
 * Remarks:     Gets the DCD line state
 * Parameters:  hport - the OS handle of opened uart port
 * Returns:     0/1 - DSR ON/OFF, -1 on error
 */
int osUartGetDCD( void* hport )
{
DWORD state;
        if( !GetCommModemStatus( hport, &state ) )return -1;
return ( state & MS_RLSD_ON ) ? 1: 0;
}
/*******************************************************************************
 *      osUartGetRI()
 * Remarks:     Gets the RING line state
 * Parameters:  hport - the OS handle of opened uart port
 * Returns:     0/1 - DSR ON/OFF, -1 on error
 */
int osUartGetRI( void* hport )
{
DWORD state;
        if( !GetCommModemStatus( hport, &state ) )return -1;
return ( state & MS_RING_ON ) ? 1: 0;
}
#undef MAXUPORTS
