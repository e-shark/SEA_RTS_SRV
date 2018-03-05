#ifndef __CUTILS_H
#define __CUTILS_H
#include<stdio.h>
/*
	vpr
        03.05.07

	Project:	Any
	Module:		Misc. service "ANSI-C" functions.
*/
/*------------------------------------------------------------------------------
 	Function reverses the byte order of continuous memory dump
------------------------------------------------------------------------------*/
void mem_revbytes(unsigned char*mem, unsigned cnt);

/*	
	Here are the definitions for porting code written for CPU 
	with little-endian byte order
	onto CPU with big-endian byte order
	vpr,07.06.2013
	
 */
#if defined _BIGENDIAN_CPU_
#define cule2be16(x)	mem_revbytes((unsigned char*)x,2)	
#define cule2be32(x)	mem_revbytes((unsigned char*)x,4)
#define cule2be64(x)	mem_revbytes((unsigned char*)x,8)
#else
#define cule2be16(x)
#define cule2be32(x)
#define cule2be64(x)
#endif
/*------------------------------------------------------------------------------
	Function reverses bits in byte b.
------------------------------------------------------------------------------*/
unsigned char revbyte(unsigned char b);
/*------------------------------------------------------------------------------
	Function calculates and
        Returns times difference in seconds between two unix times.
------------------------------------------------------------------------------*/
unsigned long GetTimeDiff(unsigned long oldTime,unsigned long newTime);
/*------------------------------------------------------------------------------
	Function delets all spaces from string in buf[].
        Returns final string length.
------------------------------------------------------------------------------*/
int strtrim(char*buf);
/*------------------------------------------------------------------------------
	Function sets pointer of text file fin to offset of first string
        behind secname string.
        Returns 1 in success, 0 otherwise.
------------------------------------------------------------------------------*/
unsigned SeekFileSection(FILE*fin,char*secname);
/*------------------------------------------------------------------------------
	"ANSI C" - Analog of Win32 GetPrivateProfileString().
------------------------------------------------------------------------------*/
unsigned cuGetPrivateProfileString(
	char*secname,
	char*keyname,
        char*defstr,
        char*retstr,
        unsigned retstrsz,
        char*fname);
/*------------------------------------------------------------------------------
	"ANSI C" - Analog of Win32 GetPrivateProfileInt().
------------------------------------------------------------------------------*/
unsigned cuGetPrivateProfileInt(
	char*secname,
        char*keyname,
        int defval,
        char*fname);
/*------------------------------------------------------------------------------
	Parses the cmdline c-string, fills in array argv[] by
        pointers to substrings of cmdline, divided by delimiter character.
        Replaces all delimiter characters in cmdline on zero byte.
------------------------------------------------------------------------------*/
int cuParseCmdLine( char *cmdline, char* argv[], char delimiter, unsigned maxpars );

#endif
