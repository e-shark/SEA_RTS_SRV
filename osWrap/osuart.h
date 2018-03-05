#ifndef OSUART_H
#define OSUART_H
/* 160214, vpr
 *                      UART Library
 *
 * Here is a set of simple functions for dealing with UART asynchronously
 * under Windows & nix operating system
 */
 
#define RXCLEAR 1       // used in sel parameter of osUartFlush()
#define TXCLEAR 2       // used in sel parameter of osUartFlush()

#ifdef __cplusplus
  extern "C" {
#endif
void* osUartOpen( char*portname, char*mode );
int osUartSetMode( void* hport, char*mode );
int osUartClose( void* hport );
int osUartRead( void* hport, unsigned char*buf, unsigned len, unsigned tout );
int osUartWrite( void* hport, unsigned char*buf, unsigned num, unsigned tout );
int osUartFlush( void* hport, int sel );

int osUartSetDTR( void* hport, int state );
int osUartSetRTS( void* hport, int state );
int osUartSetBreak( void* hport, unsigned tout );
int osUartResetBreak( void* hport );

int osUartGetCTS( void* hport );
int osUartGetDSR( void* hport );
int osUartGetDCD( void* hport );
int osUartGetRI( void* hport );
#ifdef __cplusplus
  }
#endif

#endif

