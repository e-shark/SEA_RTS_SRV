#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#pragma hdrstop

#if defined _LINUX_
char* strupr(char *s);
#endif

/*
	vpr
        28.07.05

	Project:	kkpIO
	Module:		Misc. service functions.
        All module functions are written in ANSI-C
*/
/*------------------------------------------------------------------------------
 *	June 2013, vpr
 *
 *	mem_revbytes()
 *
 *	Function reverses the byte order of continuous memory dump
 *----------------------------------------------------------------------------*/
void mem_revbytes(unsigned char*mem, unsigned cnt)
{
unsigned i;
unsigned char bs,be;

for( i = 0; i < cnt/2; i++ )
{
	bs	=	mem[i];
	be	=	mem[cnt-1-i];
	mem[cnt-1-i] =	bs;
	mem[i]	=	be;
}
}
/*------------------------------------------------------------------------------
	Меняет порядок бит в байте на обратный
------------------------------------------------------------------------------*/
unsigned char revbyte(unsigned char b)
{
int i;
unsigned char b1=0;
unsigned char m=0x1,m1=0x80;
	for(i=0;i<8;i++)
    {
    	if(b&m)b1|=m1;
        m<<=1; m1>>=1;
    }
return b1;
}
/*******************************************************************************
        Вычисляет разность двух DWORD счетчиков с учетом перехода значений счетчиков
        через 0
*******************************************************************************/
unsigned long GetTimeDiff(unsigned long oldTime,unsigned long newTime)
{
        if(newTime>=oldTime)return newTime-oldTime;
        else return newTime+(0xffffffff-oldTime);
}
/*******************************************************************************
 *	strtrim()
 *	Parameters:
 *		buf - указатель на строку, IN OUT
 *	Убирает все пробелы, символы табуляции, конца строки и перевода каретки,
 *	Убирает комментарии, начинающиеся символами "//".
 *	Returns:
 *		длину строки;
 ******************************************************************************/
int strtrim(char*buf)
{
int i;
char c,*p;

i=0;
while((c=*(buf+i))!=0)
{
	if(c==' '||c=='\t'||c=='\n' || c=='\r')
        {
        	if(*(buf+i+1))strcpy(buf+i,buf+i+1);
		else *(buf+i)=0;
        }
	else i++;
}
if((p=strstr(buf,"//"))!=NULL)*p=0;
return strlen(buf);
}
/*******************************************************************************
 *	SeekFileSection()
 *	Parameters:
 *		fin - дескриптор файла, IN
 *              secname - C-строка имени секции, любая строка, без учета регистра
 *	Ищет в текстовом файле секцию secname, начиная поиск с текущей позиции
 *      файлового указателя.
 *	Устанавливает файловый указатель на первую строку секции.
 *      Файл должен быть открыт в потоковом режиме ф-ей fopen().
 *	Returns:
 *		1 - on success;
 *              0 - section was not found
 ******************************************************************************/
unsigned SeekFileSection(FILE*fin,char*secname)
{
char buf[1000];

while( fgets(buf,sizeof(buf),fin))
        if(strtrim(buf))
                if(strcmp(secname,strupr(buf))==0)return 1;
return 0;
}
/*******************************************************************************
	cuGetPrivateProfileString()
        Кросс-платформенный "C" - аналог Win32 GetPrivateProfileString() - кроме поддержки UNICODE
*******************************************************************************/
unsigned cuGetPrivateProfileString(char*asecname,char*akeyname,char*defstr,char*retstr,unsigned retstrsz,char*fname)
{
unsigned sz;
FILE*fin;
char buf[1000],buf1[1000],*p;
char secname[900], keyname[900];

if((retstr==0) || (retstrsz==0))		return 0;
if((asecname==0) || (akeyname==0) || (fname==0))	goto defcpy;
if( (fin=fopen(fname,"rt")) == 0)		goto defcpy;
if( ( strlen(asecname) == 0 ) || ( strlen(akeyname) == 0 ) ) goto defcpy;

memset(secname, 0 , sizeof(secname));
memset(keyname, 0, sizeof(keyname));
strncpy(secname, asecname, sizeof(secname)-1);
strncpy(keyname, akeyname, sizeof(secname)-1);

strupr(secname);
strupr(keyname);

/* Looking for section...*/
while( fgets(buf,sizeof(buf),fin))
{
	strtrim(buf);
        strupr(buf);
	if((buf[0]=='[') && strchr(buf,']'))
        	if(strstr(buf,secname))break;
}
/* Looking for key...*/
while( fgets(buf,sizeof(buf),fin))
{
	strcpy(buf1,buf);	/*Copy of original string*/
	strtrim(buf);
        strupr(buf);
	if(strchr(buf,'[')){fclose(fin);goto defcpy;}
        if( NULL == ( p = strchr( buf, '=' ) ) )continue;       // 170626,vpr-bug fixing
        *p=0;                                                   // 170626,vpr-bug fixing
        if(strcmp(buf,keyname)==0)                              // 170626,vpr-bug fixing  
        {
        	p=strchr(buf1,'=');
                if(p==0)continue;
                p++;
                sz=strlen(p);
        	if(sz >= retstrsz){sz=retstrsz-1;retstr[sz]=0;}
        	strcpy(retstr,p);
                while(0!=(p=strchr(retstr,'\n')))*p=0;
                while(0!=(p=strchr(retstr,'\r')))*p=0;
                fclose(fin);
                return sz;
        }
}
fclose(fin);
defcpy:
if(defstr)
{
        	sz=strlen(defstr);
        	if(sz >= retstrsz){sz=retstrsz-1;retstr[sz]=0;}
        	strcpy(retstr,defstr);
                return sz;
}
return 0;
}
/*******************************************************************************
	cuGetPrivateProfileInt()
        Кросс-платформенный "C" - аналог Win32 GetPrivateProfileInt() - кроме поддержки UNICODE
*******************************************************************************/
unsigned cuGetPrivateProfileInt(char*secname,char*keyname,int defval,char*fname)
{
char str[1000];
if(cuGetPrivateProfileString(secname,keyname,0,str,sizeof(str),fname))return atoi(str);
return defval;
}
/*******************************************************************************
	cuParseCmdLine( )
Remarks:
	Parses the cmdline c-string, fills in array argv[] by
        pointers to substrings of cmdline, divided by delimiter character.
        Replaces all delimiter characters in cmdline on zero byte.
Parameters:
        cmdline - pointer to 0-terminated ANSI c-string
        argv - pointer to array of pointers to character strings
        delimiter - delimiter character
        maxpars - size of the argv[] array
Returns:
        Number of substrings in  cmdline
*******************************************************************************/
int cuParseCmdLine( char *cmdline, char* argv[], char delimiter, unsigned maxpars )
{
unsigned argc = 0;
char *p;
        if( (!cmdline) || (!argv) || (!maxpars) )return 0;
        p = cmdline;
        argv[ argc++ ] = p;
        if( argc >= maxpars ) return argc;
        while( NULL != ( p = strchr( p, delimiter ) ) )
        {
                *p++=0;
                if( *p != delimiter )argv[ argc++ ] = p;
                if( argc >= maxpars ) break;
        }
        return argc;
}
/*----------------------------------------------------------------------------*/

