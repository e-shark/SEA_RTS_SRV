/* os_defs.h
 *	OS constants and definitions
 *
 *      Copyright (c) 2003-2007 by KHARTEP Ltd.
 *      All Rights Reserved.
 *
 */
/* $Revision:   1.01.0407  $ */
#ifndef ___OSDEFS_H
#define ___OSDEFS_H


/*#define _WIN32_*/
/*#define _DOS16_*/
/*#define _LINUX_*/
/*------------------------------Misc. declarations----------------------*/
#ifndef NULL
#define NULL	0
#endif

#ifndef FALSE
#define FALSE	0
#endif

#ifndef TRUE
#define TRUE	1
#endif

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef CONST
#define CONST	const
#endif

#ifndef VOID
#define VOID void
#endif

typedef int             INT;
typedef unsigned int    UINT;
typedef unsigned int    *PUINT;


typedef char 		CHAR;
typedef short 		SHORT;
typedef long 		LONG;

typedef unsigned long 	ULONG;
typedef ULONG 		*PULONG;
typedef unsigned short 	USHORT;
typedef USHORT 		*PUSHORT;
typedef unsigned char 	UCHAR;
typedef UCHAR 		*PUCHAR;
typedef char 		*PSZ;

typedef CHAR 		*PCHAR;
typedef CHAR 		*LPCH;
typedef CHAR 		*PCH;

typedef float          	FLOAT;
typedef FLOAT           *PFLOAT;
typedef double		DOUBLE;

typedef int             BOOL;

typedef unsigned char   BYTE;
typedef unsigned short  WORD;
typedef unsigned long   DWORD;
typedef BOOL        	*PBOOL;
typedef BOOL         	*LPBOOL;
typedef BYTE        	*PBYTE;
typedef BYTE         	*LPBYTE;
typedef int       	*PINT;
typedef int          	*LPINT;
typedef WORD 	       	*PWORD;
typedef WORD         	*LPWORD;
typedef long         	*LPLONG;
typedef DWORD      	*PDWORD;
typedef DWORD        	*LPDWORD;
typedef void         	*LPVOID;
typedef CONST void   	*LPCVOID;
/*------------------------------Win32 environment-----------------------*/
#if defined _WIN32_
/*------------------------------DOS16 environment-----------------------*/
#elif defined _DOS16_
 typedef unsigned char 	BYTE;
 typedef unsigned short	WORD;
 typedef unsigned long	DWORD;
/*------------------------------Linux environment-----------------------*/
#elif defined _LINUX_

typedef long int DCB;
typedef void*	HANDLE;
#define INVALID_HANDLE_VALUE ((void*)(-1))
#define _timezone timezone
#define pascal

/*------------------------------Default Intel x86 32bit environment-----*/
#else

#endif

#endif
