/*----------------------------------------------------------------------------*/
#include <time.h>
#include<fcntl.h>
#include <string.h>
#include <stdlib.h>
#pragma hdrstop

#include "cutils.h"
#include "oswr.h"
#include "v2mcnf.h"
#include "v2mdapi.h"
/*------------------------------------------------------------------------------
 *	Сентябрь 1998г.
 *	Прохватилов В.Ю.
 *	Проект - драйвер FIX32 версии 6.XX - 7.XX
 *	Модуль конфигурации ввода/вывода
 *	Функции:
 *----------------------------------------------------------------------------*/
int ReadConfig(char*szCfgFileName,DWORD flags);

/*-----------------------------------------------------------Глобальные данные*/
#if defined _LINUX_     // 05.11.10 - Область видимости для Win-7...
static char*szMmfName="v2mmmf";	        /* Имя memory mapped file*/
#else
static char*szMmfName="Global\\v2mmmf";	/* Имя memory mapped file*/
#endif

unsigned char*pMmf;       	/* Указатель на начало оконного представления mmf*/
void*hMmf;	                /* handle объекта memory mapped file*/

Tcheader SWH;   		/* Буфер под считывание заголовка файла конфигурации*/
Tportdef*pdefs;			/* Указатель на массив структур описаний портов*/
KP_Table_Rec*kp;		/* Указатель на массив структур описаний КП*/
TM_Table_Rec*tm;		/* Указатель на массив структур описаний сигналов*/
/*------------------------------------------------------------------------------
 *	June 2013, vpr
 *
 *	v2mReverse()
 *
 *	Fix problem of binary configuration file reading (v2m) 
 *	for the processor architectures with  big-endian byte order
 *
 * 	Function reverses the byte order  of binary configuration file (v2m),
 *	that had been read into the memory dump
 *----------------------------------------------------------------------------*/

static int v2mReverse( Tcheader*	swh, 
		Tportdef*	pdefs,	unsigned np, 
		KP_Table_Rec*	kp,	unsigned nkp )
{
unsigned i,j;

//if( (np > MAXPORTS ) || ( nkp > MAXKP ) ) 	return -2;
/*----------------------------------------------------Reverse Tcheader--*/
if( swh )
{
 mem_revbytes( (unsigned char*)(&swh->sign),	sizeof( swh->sign 	));
 if( swh->sign != CFSIGN ) return -2;
 mem_revbytes( (unsigned char*)(&swh->num), 	sizeof( swh->num 	));
 mem_revbytes( (unsigned char*)(&swh->ports), 	sizeof( swh->ports 	));
 mem_revbytes( (unsigned char*)(&swh->flags), 	sizeof( swh->flags 	));
 mem_revbytes( (unsigned char*)(&swh->KPtSize), sizeof( swh->KPtSize));
 mem_revbytes( (unsigned char*)(&swh->TMtSize), sizeof( swh->TMtSize));
}

/*----------------------------------------------------Reverse Tportdef[]--*/
if( pdefs ) for( i = 0; i < np; i++ )
{
 mem_revbytes( (unsigned char*)(&pdefs->tout),		sizeof( pdefs->tout));
 mem_revbytes( (unsigned char*)(&pdefs->tconnect),	sizeof( pdefs->tconnect));
 pdefs+=1;
}

/*----------------------------------------------------Reverse KP_Table_Rec[]--*/
if( kp ) for( i = 0; i < nkp; i++ )
{
 mem_revbytes( (unsigned char*)(&kp->skpnum),	sizeof( kp->skpnum));
 mem_revbytes( (unsigned char*)(&kp->kpnum),	sizeof( kp->kpnum));
 mem_revbytes( (unsigned char*)(&kp->outpmask),	sizeof( kp->outpmask));
 for( j = 0; j < MAXBOARDS; j++ )
 {
 mem_revbytes( (unsigned char*)(&kp->bsnum[j]),	sizeof( kp->bsnum[j]));
 }
 mem_revbytes( (unsigned char*)(&kp->tout),	sizeof( kp->tout));
 mem_revbytes( (unsigned char*)(&kp->treq),	sizeof( kp->treq));
 mem_revbytes( (unsigned char*)(&kp->texch),	sizeof( kp->texch));
 kp+=1;
}
return 0;
}
/*------------------------------------------------------------------------------
 *
 *	ReadConfig()
 *	------------
 *		Модуль интерфейса с PLC может быть запущен совместно с драйвером FIX-
 *	и тогда он должен использовать mmf конфигурации драйвера-для такого запуска
 *	в ReadCjnfig() нужно передавать
 *			szCfgFileName=NULL,
 *	ВНИМАНИЕ!! При запуске модуля с szCfgFileName=NULL, когда не запущен драйвер FIX
 *	ReadCjnfig() возвращает 0, и завершает работу.
 *		Модуль интерфейса с PLC может быть запущен автономно-и тогда в ReadCjnfig()
 *	нужно передавать корректное имя файла конфигурации-модуль скопирует его в mmf.
 *	Однако если при попытке автономного запуска уже запущен драйвер FIX-будет
 *	использована его текущая конфигурация!!!
 *	Возвращает:
 *	-4 - системная ошилка создания mmf
 *	-3 - mmf уже существует, но не  идентичен конфигурационному файлу
 *	-2 - ошибка структуры файла конфигурации
 *	-1 - ошибка чтения файла конфигурации
 *	0 - файл конфигурации не задан, и mmf не найден в ОЗУ
 *	1 - OK! Mmf создан, конфигурация считана в mmf
 *	2 - Mmf существует на момент обращения, конфигурация берется из него
 *----------------------------------------------------------------------------*/
int ReadConfig(char*szCfgFileName,DWORD flags)
{
int i,j,k;
int hFile;
DWORD bn;		/* Количество прочитанных байт*/
DWORD configsz;		/*Размер memory mapped file*/
BOOL nextinstance=FALSE;/* Признак экземпляра*/
/*---------------------------------------------------Чтение файла конфигурации*/
memset(&SWH,0,sizeof(SWH));

if(szCfgFileName!=NULL)
{
	/*-----------------------------------------Открытие файла конфигурации*/
        hFile=osOpenFile(szCfgFileName,O_RDONLY);
        if(hFile<0)return -1;
	/*----------------------------------------------------Чтение Заголовка*/
        bn=osReadFile(hFile,&SWH,sizeof(SWH));
        osCloseFile(hFile);
	if(bn!=sizeof(SWH))
	{
    		memset(&SWH,0,sizeof(SWH));
    		return -1;
	}
#if defined _BIGENDIAN_CPU_
        if(v2mReverse(&SWH,0,0,0,0)<0)return -2;
#endif
	if(SWH.sign!=CFSIGN)
	{
    		memset(&SWH,0,sizeof(SWH));
    		return -2;
	}
	/*----------------------------Считаем размер MMF по файлу конфигурации*/
	configsz=sizeof(Tcheader)
			+sizeof(Tportdef)*SWH.ports
            		+sizeof(KP_Table_Rec)*SWH.KPtSize
                	+sizeof(TM_Table_Rec)*SWH.TMtSize;
        /*--------------------------------------------------------Создание mmf*/
        pMmf=osCreateMmf(szMmfName,configsz,&nextinstance,&hMmf);
        if(pMmf==NULL)
        {
    	        memset(&SWH,0,sizeof(SWH));
                return -4;
        }
}
else
{
		        /* В случае,если имя файла конфигурации не задано-то необходимо
			   наличие в памяти открытого mmf, и тогда переданный
                           CreateMmh() размер игнорируется,если же файла mmf нет-
                           необходимо прекращать работу-поэтому задаем произвольный размер
                           mmf-если mmf нет в памяти и он будет создан с указанным размером-
                           мы его тут же закроем и завершим работу*/
        /*--------------------------------------------------------Открытие mmf*/
        pMmf=osOpenMmf(szMmfName,&hMmf);
        if(pMmf==NULL)return 0;
        nextinstance=TRUE;
}
/*---------------------------------------------------Первая копия dll драйвера*/
/*	Читаем конфигурацию в Mmf*/
if(!nextinstance)
{
	memset(pMmf,0,configsz);
    	i=sizeof(Tcheader)+sizeof(Tportdef)*SWH.ports+sizeof(KP_Table_Rec)*SWH.KPtSize;
        hFile=osOpenFile(szCfgFileName,O_RDONLY);
        if(hFile<0)
        {
    		memset(&SWH,0,sizeof(SWH));
                osCloseMmf(pMmf,&hMmf);
    		pMmf=0;
                return -1;
        }
        bn=osReadFile(hFile,pMmf,i);
        osCloseFile(hFile);
	if(bn!=(DWORD)i)
    	{
    		memset(&SWH,0,sizeof(SWH));
                osCloseMmf(pMmf,&hMmf);
    		pMmf=0;
    		return -1;
	}
	/*-------------------------------------Установка указателей на таблицы*/
	pdefs=(Tportdef*)(pMmf+sizeof(Tcheader));			/* Таблица портов*/
	kp=(KP_Table_Rec*)((PBYTE)pdefs+sizeof(Tportdef)*SWH.ports);	/*Таблица КП*/
	tm=(TM_Table_Rec*)((PBYTE)kp+sizeof(KP_Table_Rec)*SWH.KPtSize);	/*Таблица ТМ*/
#if defined _BIGENDIAN_CPU_
        if(v2mReverse((Tcheader*)pMmf,pdefs,SWH.ports,kp,SWH.KPtSize)<0)return -2;
#endif
	/*--------------------Установка смещений буферов плат в таблице телемеханики*/
	k=0;
	for(i=0;i<SWH.KPtSize;i++)
    	{
    		for(j=0;j<kp[i].nboards;j++)
        	{
        		kp[i].bfrom[j]=k;
            		k+=kp[i].bsnum[j];
        	}
    	}
    	/*------------------------------Подготовка строк инициализации модемов*/
    	for(i=0;i<SWH.KPtSize;i++)      strcat(kp[i].callstr,"\r");
    	for(i=0;i<SWH.ports;i++){       strcat(pdefs[i].initstr[0],"\r");
                                        strcat(pdefs[i].initstr[1],"\r");}
}
/*--------------------------------------------Уже не первая копия dll драйвера*/
else
{
	if(szCfgFileName!=NULL)if(memcmp(pMmf,&SWH,sizeof(SWH))!=0)
    	{
exit:      	memset(&SWH,0,sizeof(SWH));
                osCloseMmf(pMmf,&hMmf);
     		pMmf=0;
      		return(-3);
    	}
        if(((Tcheader*)pMmf)->sign!=CFSIGN)goto exit;
	memcpy(&SWH,pMmf,sizeof(SWH));
	/*-------------------------------------------Установка указателей на таблицы*/
	pdefs=(Tportdef*)(pMmf+sizeof(Tcheader));			/* Таблица портов*/
	kp=(KP_Table_Rec*)((PBYTE)pdefs+sizeof(Tportdef)*SWH.ports);	/*Таблица КП*/
	tm=(TM_Table_Rec*)((PBYTE)kp+sizeof(KP_Table_Rec)*SWH.KPtSize);	/*Таблица ТМ*/
}
/*-------------------------------------Начальное обнуление флагов изменения ТУ*/
if(flags&1)for(i=0;i<SWH.TMtSize;i++)tm[i].flag=0;
/*-----------------------------------------------------------------------Выход*/
if(nextinstance){return 2;}
return 1;
}
/*------------------------------------------------------------------------------
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
                        DIT API - Common Functions
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
*/
/*------------------------------------------------------------------------------
	AttachDataSource()
	AttachDataSourceEx()
	------------------
	Attaches to v2m DIT.
------------------------------------------------------------------------------*/
BOOL AttachDataSource(void)
{
	if(!GetSrcStatus())ReadConfig(NULL,0);
	return(GetSrcStatus());
}
int AttachDataSourceEx(char*fname,DWORD flags)
{
	return ReadConfig(fname,flags);
}
/*------------------------------------------------------------------------------
	DetachDataSource()
	------------------
	Detaches from v2m DIT.
------------------------------------------------------------------------------*/
void DetachDataSource(void){osCloseMmf(pMmf,&hMmf);pMmf=0;}
/*------------------------------------------------------------------------------
	GetSrcStatus()
	--------------
        Gets DIT status.
        Returns: TRUE 	- v2m DIT data source is ready
        	FALSE 	- v2m DIT data source is not accessible
------------------------------------------------------------------------------*/
BOOL GetSrcStatus(void){return((pMmf!=NULL));}
/*------------------------------------------------------------------------------
	SaveDITToFile()
        ---------------
------------------------------------------------------------------------------*/
BOOL SaveDITToFile(char*fname)
{
int i;
int hFile;
int bn;				/* Количество записанных байт*/

if((GetFlags()&1)==0)return FALSE;
/*-----------------------------------------------------------------Делаем Файл*/
hFile=osCreateFile(fname,0);
if(hFile<0)return FALSE;
/*--------------------------------------Сохранение образа mmf в дисковом файле*/
i=sizeof(Tcheader)+sizeof(Tportdef)*GetPortsQuantity()+sizeof(KP_Table_Rec)*GetDevsQuantity()+sizeof(TM_Table_Rec)*GetTmRecordsQuantity();
bn=osWriteFile(hFile,pMmf,i);
osCloseFile(hFile);
if(bn!=i)return FALSE;
return TRUE;
}
/*------------------------------------------------------------------------------
	RestoreDITFromFile()
        --------------------
------------------------------------------------------------------------------*/
BOOL RestoreDITFromFile(char*fname)
{
int i;
long fs;
int hFile;
int bn;					/* Количество прочитанных байт*/

Tcheader*SWH1;				/* Указатель на буфер под считывание заголовка файла конфигурации*/
Tportdef*pdefs1;			/* Указатель на массив структур описаний портов*/
KP_Table_Rec*kp1;			/* Указатель на массив структур описаний КП*/
TM_Table_Rec*tm1;			/* Указатель на массив структур описаний сигналов*/

if((GetFlags()&1)==0)return FALSE;
/*------------------------Открытие и чтение дискового файла дампа общей памяти*/
hFile=osOpenFile(fname,O_RDONLY);
if(hFile<0)return FALSE;
fs=osGetFileSize(hFile);
if(fs==-1){osCloseFile(hFile);return FALSE;}
i=sizeof(Tcheader)+sizeof(Tportdef)*GetPortsQuantity()+sizeof(KP_Table_Rec)*GetDevsQuantity()+sizeof(TM_Table_Rec)*GetTmRecordsQuantity();
if(fs!=i){osCloseFile(hFile);return FALSE;}
SWH1=(Tcheader*)malloc(fs);
if(SWH1==0){osCloseFile(hFile);return FALSE;}
bn=osReadFile(hFile,SWH1,fs);
if(bn!=fs){osCloseFile(hFile);free(SWH1);return FALSE;}
osCloseFile(hFile);
/*---------------------------------------------Проверка соответствия структуры*/
if(memcmp(pMmf,SWH1,sizeof(SWH1))!=0)
{
	free(SWH1);
        return FALSE;
}
/*---------------------------------------------Установка указателей на таблицы*/
pdefs1=(Tportdef*)((unsigned char*)SWH1+sizeof(Tcheader));			/*Таблица портов*/
kp1=(KP_Table_Rec*)((unsigned char*)pdefs1+sizeof(Tportdef)*SWH1->ports);	/*Таблица КП*/
tm1=(TM_Table_Rec*)((unsigned char*)kp1+sizeof(KP_Table_Rec)*SWH1->KPtSize);	/*Таблица ТМ*/
/*-------------------------------------Начальное обнуление флагов изменения ТУ*/
for(i=0;i<SWH1->TMtSize;i++)tm1[i].flag=0;
/*--------------------------------------------Копирование таблицы телемеханики*/
memcpy(tm,tm1,sizeof(TM_Table_Rec)*GetTmRecordsQuantity());
free(SWH1);
return TRUE;
}
/*------------------------------------------------------------------------------
 *	GetFlags()
 *----------------------------------------------------------------------------*/
WORD GetFlags(void)
{
        if(!GetSrcStatus())return 0;
        return SWH.flags;
}
WORD GetSysNo(void)
{
        if(!GetSrcStatus())return 0;
        return SWH.num;
}
/*------------------------------------------------------------------------------
	SetDebugMode()
        Parameters:
                mode    - debugging mode
                0 - debugging is OFF
                1 - first debugging level (errors & system messages)
                2 - second debugging level (level1 + logging)
                3 - 3 debugging level (level1 + debugging)
                4 - forth debugging level (level1 + logging + debugging)
                5 - system debug
        Returns: TRUE - operation completes successfully
        	 FALSE - error
------------------------------------------------------------------------------*/
BOOL SetDebugMode(BYTE mode)
{
        if(!GetSrcStatus())return FALSE;
        switch(mode)
        {
        default:        /*Отключение отладки и журнализации*/
                SWH.flags&=(~0xC0);
                SetDevsOnDebug(0);
                SetPortsOnDebug(0);break;
        case 1:
        case 6: /*Только сообщения об ошибках и системных событиях*/
                SWH.flags|=0x80;
                SetDevsOnDebug(0);
                SetPortsOnDebug(0);break;
        case 2: /*Журнализация по всем устройствам*/
                SWH.flags|=0x40;
                SetDevsOnDebug(1);
                SetPortsOnDebug(1);break;
        case 3: /*Отладка по всем устройствам*/
                SWH.flags|=0x80;
                SetDevsOnDebug(1);
                SetPortsOnDebug(1);break;
        case 4: /*Отладка и журнализация по всем устройствам*/
                SWH.flags|=0xC0;
                SetDevsOnDebug(1);
                SetPortsOnDebug(1);break;
        }
return TRUE;
}
/*------------------------------------------------------------------------------
 *	IsDebugging()
 *----------------------------------------------------------------------------*/
BOOL IsDebugging()
{
        if(!GetSrcStatus())return FALSE;
        return (SWH.flags&0x80)?TRUE:FALSE;
}
/*------------------------------------------------------------------------------
 *	IsLogging()
 *----------------------------------------------------------------------------*/
BOOL IsLogging()
{
        if(!GetSrcStatus())return FALSE;
        return (SWH.flags&0x40)?TRUE:FALSE;
}
/*------------------------------------------------------------------------------
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
                        DIT API - PORT functions
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
*/
WORD GetPortsQuantity(void)
{
        if(!GetSrcStatus())return 0;
        return SWH.ports;
}
/*------------------------------------------------------------------------------
	GetPortHandle()
        Parameters:
                PortNo         - port number
        Returns: >=0 	- port handle
        	-1 	- port not found
------------------------------------------------------------------------------*/
int GetPortHandle(int PortNo)
{
int i;
        if(!GetSrcStatus())return -1;
        for(i=0;i<SWH.ports;i++)if(pdefs[i].port==PortNo)return(i);
        return -1;
}
int GetPortNo(int hPort)
{
        if(!GetSrcStatus())return -1;
        if((hPort<0) || (hPort>SWH.ports))return -1;
        return pdefs[hPort].port;
}
BYTE GetPortSpeedIdx(int pidx)
{
        if(!GetSrcStatus())return 0;
        if((pidx<0) || (pidx>=SWH.ports))return 0;
        return pdefs[pidx].baud_ndx;
}
BYTE GetPortUARTParams(int pidx)
{
        if(!GetSrcStatus())return 0;
        if((pidx<0) || (pidx>=SWH.ports))return 0;
        return pdefs[pidx].params;
}
BYTE GetPortFlags(int pidx)
{
        if(!GetSrcStatus())return 0;
        if((pidx<0) || (pidx>=SWH.ports))return 0;
        return pdefs[pidx].flags;
}
BYTE GetPortType(int pidx)
{
        if(!GetSrcStatus())return 0;
        if((pidx<0) || (pidx>=SWH.ports))return 0;
        return pdefs[pidx].type;
}
BYTE GetPortProto(int pidx)
{
        if(!GetSrcStatus())return 0;
        if((pidx<0) || (pidx>=SWH.ports))return 0;
        return pdefs[pidx].protocol;
}
unsigned GetPortTout(int pidx)
{
        if(!GetSrcStatus())return 0;
        if((pidx<0) || (pidx>=SWH.ports))return 0;
        return pdefs[pidx].tout;
}
unsigned GetPortConWaitTime(int pidx)
{
        if(!GetSrcStatus())return 0;
        if((pidx<0) || (pidx>=SWH.ports))return 0;
        return pdefs[pidx].tconnect;
}
unsigned GetPortElapsedTime(int pidx)
{
        if(!GetSrcStatus())return 0;
        if((pidx<0) || (pidx>=SWH.ports))return 0;
        return pdefs[pidx].elapsedtime;
}
unsigned IncPortElapsedTime(int hPort)
{
        if(!GetSrcStatus())return 0;
        if((hPort<0) || (hPort>=SWH.ports))return 0;
        return (++pdefs[hPort].elapsedtime);
}
unsigned IncPortStat(int pidx,int sn)
{
        if(!GetSrcStatus())return 0;
        if((pidx<0) || (pidx>=SWH.ports))return 0;
        if((sn>=0) && (sn<MAXSTAT))return(++pdefs[pidx].stat[sn]);
        else return 0;
}
unsigned GetPortStat(int pidx,int sn)
{
        if(!GetSrcStatus())return 0;
        if((pidx<0) || (pidx>=SWH.ports))return 0;
        if((sn>=0) && (sn<MAXSTAT))return(pdefs[pidx].stat[sn]);
        else return 0;
}
BOOL SetPortStat(int pidx,int sn,unsigned dwValue)
{
        if(!GetSrcStatus())return FALSE;
        if((pidx<0) || (pidx>=SWH.ports))return FALSE;
        if((sn>=0) && (sn<MAXSTAT))
        {
                pdefs[pidx].stat[sn]=dwValue;
                return TRUE;
        }
        else return FALSE;
}
/*------------------------------------------------------------------------------
        ResetPortStat()
        ResetPortElapsedTime()
        IsPortOn()
        SetPortOn()
        SetPortOff()
        ---------------------
        Parameters:
                hPort   -       port handler, IN
                no      -       parameter number, IN
        Returns:
                TRUE - OK, FALSE - error.
 *----------------------------------------------------------------------------*/
BOOL ResetPortStat(int pidx,int sn)
{
        if(!GetSrcStatus())return FALSE;
        if((pidx<0) || (pidx>=SWH.ports))return FALSE;
        if((sn>=0) && (sn<MAXSTAT)){pdefs[pidx].stat[sn]=0;return TRUE;}
        else return FALSE;
}
BOOL ResetPortElapsedTime(int pidx)
{
        if(!GetSrcStatus())return FALSE;
        if((pidx<0) || (pidx>=SWH.ports))return FALSE;
        pdefs[pidx].elapsedtime=0;
        return TRUE;
}
BOOL IsPortOn(int pidx)
{
        if(!GetSrcStatus())return FALSE;
        if((pidx<0) || (pidx>=SWH.ports))return FALSE;
        return(pdefs[pidx].flags&1)?TRUE:FALSE;
}
BOOL SetPortOn(int pidx)
{
        if(!GetSrcStatus())return FALSE;
        if((pidx<0) || (pidx>=SWH.ports))return FALSE;
        pdefs[pidx].flags|=1;
        return TRUE;
}
BOOL SetPortOff(int pidx)
{
        if(!GetSrcStatus())return FALSE;
        if((pidx<0) || (pidx>=SWH.ports))return FALSE;
        pdefs[pidx].flags&=(~1);
        return TRUE;
}
BOOL SetPortOnDebug(int pidx,BYTE mode)
{
        if(!GetSrcStatus())return FALSE;
        if((pidx<0) || (pidx>=SWH.ports))return FALSE;
        pdefs[pidx].flags|=0x80;
        SWH.flags|=0x80;
        return TRUE;
}
BOOL IsPortOnDebug(int pidx)
{
        if(!GetSrcStatus())return FALSE;
        if((pidx<0) || (pidx>=SWH.ports))return FALSE;
        if(pdefs[pidx].flags&0x80)      return TRUE;
        else                            return FALSE;
}
BOOL SetPortsOnDebug(BYTE mode)
{
int i;
        if(!GetSrcStatus())return FALSE;
        for(i=0;i<SWH.ports;i++)
        {
                if(mode)pdefs[i].flags|=0x80;
                else    pdefs[i].flags&=(BYTE)(~0x80);
        }
        return TRUE;
}
/*------------------------------------------------------------------------------
        GetFirstPortDevHandle()
	GetFirstPortDevType()
        GetPortDevsNum()
	---------------------
	Gets fist port device handle/type/quantity
        Parameters:
        	hPort - 	port handle,          IN
	Returns:
        	>=0 device handle/type/quantity.
                <0  error
------------------------------------------------------------------------------*/
int GetFirstPortDevHandle(int pidx)
{
	int i;
        if(!GetSrcStatus())return -1;
	for(i=0;i<SWH.KPtSize;i++)if(kp[i].port==pidx)return i;
	return -1;
}
int GetFirstPortDevType(int pidx)
{
	int i;
        if(!GetSrcStatus())return -1;
	for(i=0;i<SWH.KPtSize;i++)if(kp[i].port==pidx)return kp[i].type;
	return -1;
}
int GetPortDevsNum(int pidx)
{
int i,n=0;
        if(!GetSrcStatus())return -1;
	for(i=0;i<SWH.KPtSize;i++)if(kp[i].port==pidx)n++;
        return n;
}
BOOL GetPortInitString(int hPort,int no,char*str)
{
        if(!GetSrcStatus())return FALSE;
        if((hPort<0) || (hPort>=SWH.ports))return FALSE;
        if((no<0)||(no>=MAXPORTINITSTRS))return FALSE;
        if(str==0)return FALSE;
        strcpy(str,pdefs[hPort].initstr[no]);
        return TRUE;
}
/*------------------------------------------------------------------------------
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
                        DIT API - Device functions
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
*/
/*------------------------------------------------------------------------------
 	GetDevsQuantity()
 -----------------------------------------------------------------------------*/
WORD GetDevsQuantity(void)
{
        if(!GetSrcStatus())return 0;
        return SWH.KPtSize;
}
/*------------------------------------------------------------------------------
        GetDevHandleBySno()
        Parameters:
                DeviceSystemNo	- system device number
        Returns: >=0 	- device handle
        	-1 	- device not found
------------------------------------------------------------------------------*/
int GetDevHandleBySno(int skpnum)
{
int i;
        if(!GetSrcStatus())return -1;
        for(i=0;i<SWH.KPtSize;i++)if(kp[i].skpnum==skpnum)return(i);
        return -1;
}
/*------------------------------------------------------------------------------
        GetRDevHandleBySno()
        Parameters:
                DeviceSystemNo	- system device number
        Returns: >=0 	- device handle
        	-1 	- device not found
------------------------------------------------------------------------------*/
int GetRDevHandleBySno(int skpnum)
{
int i,j;
        if(!GetSrcStatus())return -1;
        for(i=0,j=0;i<SWH.KPtSize;i++)if(kp[i].skpnum==skpnum)
	{
		if(j)return(i);
		j++;
	}
        return -1;
}
/*------------------------------------------------------------------------------
	GetDevHandleByNo()
        Parameters:
        	hPort 		- port handle
                DeviceNo	- device communication number
                DeviceType      - device type
        Returns: >=0 	- device handle
        	-1 	- device not found
------------------------------------------------------------------------------*/
int GetDevHandleByNo(int pidx,int kpnum,int type)
{
int i;
        if(!GetSrcStatus())return -1;
        if(pidx>=SWH.ports)return -1;
        for(i=0;i<SWH.KPtSize;i++)if((kp[i].kpnum==kpnum)&&(kp[i].port==pidx)&&(kp[i].type==type))return(i);
        return -1;
}
/*------------------------------------------------------------------------------
 *      GetDevSno()
 *      Определяет системный номер КП по его индексу в таблице КП
 *      Return:
 *      Системный номер КП
 *      -1 - КП не найден
 *----------------------------------------------------------------------------*/
int GetDevSno(int hDevice)
{
        if(!GetSrcStatus())return -1;
        if((hDevice<0)||(hDevice>SWH.KPtSize))return -1;
        return kp[hDevice].skpnum;
}
int GetDevNo(int kpidx)
{
        if(!GetSrcStatus())return -1;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return -1;
        return kp[kpidx].kpnum;
}
int GetDevType(int kpidx)
{
        if(!GetSrcStatus())return -1;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return -1;
        return kp[kpidx].type;
}
int GetDevPortHandle(int kpidx)
{
        if(!GetSrcStatus())return -1;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return -1;
        return kp[kpidx].port;
}
/*------------------------------------------------------------------------------
        GetDevPortNo()
        Определяет номер порта КП по его индексу в таблице КП
        Return:
        Номер порта КП
        -1 - КП не найден
  ----------------------------------------------------------------------------*/
int GetDevPortNo(int hDevice)
{
        if(!GetSrcStatus())return -1;
        if((hDevice<0)||(hDevice>SWH.KPtSize))return -1;
        return pdefs[kp[hDevice].port].port;
}
int GetDevBlocksNum(int kpidx)
{
        if(!GetSrcStatus())return -1;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return -1;
        return kp[kpidx].nboards;
}
unsigned GetDevSessionTout(int kpidx)
{
        if(!GetSrcStatus())return 0;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return 0;
        return kp[kpidx].tout;
}
unsigned GetDevPollTime(int kpidx)
{
        if(!GetSrcStatus())return 0;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return 0;
        return kp[kpidx].treq;
}
unsigned GetDevPollWindowTime(int kpidx)
{
        if(!GetSrcStatus())return 0;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return 0;
        return kp[kpidx].texch;
}
WORD GetDevFlags(int kpidx)
{
        if(!GetSrcStatus())return 0;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return 0;
        return kp[kpidx].KPf;
}
WORD SetDevFlags(int kpidx,WORD fl)
{
        if(!GetSrcStatus())return 0;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return 0;
        kp[kpidx].KPf=fl;
        return kp[kpidx].KPf;
}
BYTE GetDevDiag(int kpidx,int dn)
{
        if(!GetSrcStatus())return 0;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return 0;
        if((dn>=0) && (dn<MAXDIAG))return kp[kpidx].diag[dn];
        else return 0;
}
BYTE SetDevDiag(int kpidx,int dn,BYTE newd)
{
        if(!GetSrcStatus())return 0;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return 0;
        if((dn<0) || (dn>=MAXDIAG))return 0;
        if(kp[kpidx].diag[dn]!=newd)kp[kpidx].KPf|=1;
        kp[kpidx].diag[dn]=newd;
        return kp[kpidx].diag[dn];
}
unsigned GetDevElapsedTime(int kpidx)
{
        if(!GetSrcStatus())return 0;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return 0;
        return kp[kpidx].elapsedtime;
}
unsigned IncDevElapsedTime(int hDevice)
{
        if(!GetSrcStatus())return 0;
        if((hDevice<0) || (hDevice>=SWH.KPtSize))return 0;
        return (++kp[hDevice].elapsedtime);
}
unsigned GetDevStat(int kpidx,int sn)
{
        if(!GetSrcStatus())return 0;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return 0;
        if((sn>=0) && (sn<MAXSTAT))return(kp[kpidx].stat[sn]);
        else return 0;
}
unsigned SetDevStat(int kpidx,int sn,unsigned stat)
{
        if(!GetSrcStatus())return 0;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return 0;
        if((sn>=0) && (sn<MAXSTAT))return(kp[kpidx].stat[sn]=stat);
        else return 0;
}
unsigned IncDevStat(int kpidx,int sn)
{
        if(!GetSrcStatus())return 0;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return 0;
        if((sn>=0) && (sn<MAXSTAT))return(++kp[kpidx].stat[sn]);
        else return 0;
}
unsigned DecDevStat(int kpidx,int sn)
{
        if(!GetSrcStatus())return 0;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return 0;
        if((sn>=0) && (sn<MAXSTAT))
        {
                if(kp[kpidx].stat[sn])
                        return(--kp[kpidx].stat[sn]);
        }
        return 0;
}
BOOL ResetDevStat(int kpidx,int sn)
{
        if(!GetSrcStatus())return FALSE;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return FALSE;
        if((sn>=0) && (sn<MAXSTAT)){kp[kpidx].stat[sn]=0;return TRUE;}
        else return FALSE;
}
BOOL ResetDevStats(int kpidx)
{
int i;
        if(!GetSrcStatus())return FALSE;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return FALSE;
        for(i=0;i<MAXSTAT;i++)kp[kpidx].stat[i]=0;
        return TRUE;
}
BOOL ResetDevElapsedTime(int kpidx)
{
        if(!GetSrcStatus())return FALSE;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return FALSE;
        kp[kpidx].elapsedtime=0;
        return TRUE;
}
BOOL IsDevOnLine(int kpidx)
{
        if(!GetSrcStatus())return FALSE;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return FALSE;
        return (kp[kpidx].diag[0]&1)?TRUE:FALSE;
}
BOOL IsDevHealthy(int kpidx)
{
        if(!GetSrcStatus())return FALSE;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return FALSE;
        return (kp[kpidx].diag[1])?FALSE:TRUE;
}
BOOL IsDevOnPoll(int kpidx)
{
        if(!GetSrcStatus())return FALSE;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return FALSE;
        return (kp[kpidx].diag[9]&1)?TRUE:FALSE;
}
BOOL IsDevOnAck(int kpidx)
{
        if(!GetSrcStatus())return FALSE;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return FALSE;
        return (kp[kpidx].diag[9]&2)?TRUE:FALSE;
}
BOOL IsDevOnControl(int kpidx)
{
        if(!GetSrcStatus())return FALSE;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return FALSE;
        return (kp[kpidx].diag[9]&4)?TRUE:FALSE;
}
BOOL SetDevOnPoll(int kpidx)
{
        if(!GetSrcStatus())return FALSE;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return FALSE;
        kp[kpidx].diag[9]|=1;
        return TRUE;
}
BOOL SetDevOnAck(int kpidx)
{
        if(!GetSrcStatus())return FALSE;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return FALSE;
        kp[kpidx].diag[9]|=2;
        return TRUE;
}
BOOL SetDevOnControl(int kpidx)
{
        if(!GetSrcStatus())return FALSE;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return FALSE;
        kp[kpidx].diag[9]|=4;
        return TRUE;
}
BOOL SetDevOnDebug(int kpidx,BYTE mode)
{
        if(!GetSrcStatus())return FALSE;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return FALSE;
        if(mode)        kp[kpidx].KPf|=0x80;
        else            kp[kpidx].KPf&=(~0x80);
        SWH.flags|=0x80;
        return TRUE;
}
BOOL IsDevOnDebug(int kpidx)
{
        if(!GetSrcStatus())return FALSE;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return FALSE;
        if(kp[kpidx].KPf&0x80)  return TRUE;
        else                    return FALSE;
}
BOOL SetDevsOnDebug(BYTE mode)
{
int i;
        if(!GetSrcStatus())return FALSE;
        for(i=0;i<SWH.KPtSize;i++)
        {
                if(mode)        kp[i].KPf|=0x80;
                else            kp[i].KPf&=(~0x80);
        }
        return TRUE;
}
BOOL SetDevLinkStatus(int kpidx,BYTE mode)
{
        if(!GetSrcStatus())return FALSE;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return FALSE;
        if(mode)kp[kpidx].elapsedtime=0;
        if(kp[kpidx].diag[0]!=mode)
        {
                if( mode ) ResetDevLatchedCommands(kpidx); //23.02.10 - Reset latched commands after link restoration  
                kp[kpidx].diag[0]=mode;
                kp[kpidx].KPf|=1;
                return TRUE;
        }
        return FALSE;
}
BOOL GetDevCallStr(int hDevice,char*str)
{
        if(!GetSrcStatus())return FALSE;
        if((hDevice<0) || (hDevice>=SWH.KPtSize))return FALSE;
        if(str==0)return 0;
        strcpy(str,kp[hDevice].callstr);
        return TRUE;
}
BOOL SetDevChangeFlags(int hDevice)
{
int i,hBlock,type;
        if(!GetSrcStatus())return FALSE;
        if((hDevice<0) || (hDevice>=SWH.KPtSize))return FALSE;
        for(hBlock=0;hBlock<kp[hDevice].nboards;hBlock++)
        {
                type=kp[hDevice].btype[hBlock];
                if((type==TI)||(type==TS)||(type==TII))
                        for(i=0;i<kp[hDevice].bsnum[hBlock];i++)tm[kp[hDevice].bfrom[hBlock]+i].flag=(~0);
        }
return TRUE;
}
BOOL ResetDevLatchedCommands(int hDevice)
{
int i,hBlock,type;
        if(!GetSrcStatus())return FALSE;
        if((hDevice<0) || (hDevice>=SWH.KPtSize))return FALSE;
        for(hBlock=0;hBlock<kp[hDevice].nboards;hBlock++)
        {
                type=kp[hDevice].btype[hBlock];
                if(type==TU)
                        for(i=0;i<kp[hDevice].bsnum[hBlock];i++)tm[kp[hDevice].bfrom[hBlock]+i].flag = 0;
        }
return TRUE;
}
/*------------------------------------------------------------------------------
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
                        DIT API - BLOCK functions
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
*/
/*------------------------------------------------------------------------------
	GetBlockHandle()
        Parameters:
        	hDevice		- device handle
                BlockNo         - block number
        Returns: >=0 	- device handle
        	-1 	- device not found
------------------------------------------------------------------------------*/
int GetBlockHandle(int kpidx,int bnum)
{
int i;
        if(!GetSrcStatus())return -1;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return -1;
        for(i=0;i<kp[kpidx].nboards;i++)if(kp[kpidx].bnum[i]==bnum)return(i);
        return -1;
}
int GetBlockHandleExt(int kpidx,int bnum,int type)
{
int i;
        if(!GetSrcStatus())return -1;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return -1;
        for(i=0;i<kp[kpidx].nboards;i++)
                if((kp[kpidx].bnum[i]==bnum)&&(kp[kpidx].btype[i]==type))return(i);
        return -1;
}
/*------------------------------------------------------------------------------
 *	GetBlockNo()
 *----------------------------------------------------------------------------*/
int GetBlockNo(int hDevice,int hBlock)
{
        if(!GetSrcStatus())return -1;
        if((hDevice<0) || (hDevice>=SWH.KPtSize))return -1;
        if(hBlock>=kp[hDevice].nboards)return -1;
        return kp[hDevice].bnum[hBlock];
}
int GetBtype(int kpidx,int bidx)
{
        if(!GetSrcStatus())return -1;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return -1;
        if((bidx<0) || (bidx>=kp[kpidx].nboards))return -1;
        return kp[kpidx].btype[bidx];
}
int GetBnum(int kpidx,int bidx)
{
        if(!GetSrcStatus())return -1;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return -1;
        if((bidx<0) || (bidx>=kp[kpidx].nboards))return -1;
        return kp[kpidx].bnum[bidx];
}
int GetBsnum(int kpidx,int bidx)
{
        if(!GetSrcStatus())return -1;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return -1;
        if((bidx<0) || (bidx>=kp[kpidx].nboards))return -1;
        return kp[kpidx].bsnum[bidx];
}
int GetBlockTagsNum(int kpidx,int bidx)
{
        if(!GetSrcStatus())return -1;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return -1;
        if((bidx<0) || (bidx>=kp[kpidx].nboards))return -1;
        if((kp[kpidx].btype[bidx]==TS) || (kp[kpidx].btype[bidx]==TU))return kp[kpidx].bsnum[bidx]*8;
        else return kp[kpidx].bsnum[bidx];
}
int GetBflags(int kpidx,int bidx)
{
        if(!GetSrcStatus())return -1;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return -1;
        if((bidx<0) || (bidx>=kp[kpidx].nboards))return -1;
        return kp[kpidx].bflags[bidx];
}
DWORD GetBpactime(int kpidx,int bidx)
{
        if(!GetSrcStatus())return 0;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return 0;
        if((bidx<0) || (bidx>=kp[kpidx].nboards))return 0;
        return kp[kpidx].bpactime[bidx];
}
DWORD SetBpactime(int kpidx,int bidx,DWORD t)
{
        if(!GetSrcStatus())return 0;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return 0;
        if((bidx<0) || (bidx>=kp[kpidx].nboards))return 0;
        kp[kpidx].bpactime[bidx]=t;
        return t;
}
DWORD GetBtime(int kpidx,int bidx,time_sms*tm)
{
        if(!GetSrcStatus())return 0;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return 0;
        if((bidx<0) || (bidx>=kp[kpidx].nboards))return 0;
	tm->sec=kp[kpidx].btime[bidx].sec;
        tm->msec=kp[kpidx].btime[bidx].msec;
        return kp[kpidx].btime[bidx].sec;
}
DWORD SetBtime(int kpidx,int bidx,time_sms*tm)
{
        if(!GetSrcStatus())return 0;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return 0;
        if((bidx<0) || (bidx>=kp[kpidx].nboards))return 0;
	kp[kpidx].btime[bidx].sec=tm->sec;
        kp[kpidx].btime[bidx].msec=tm->msec;
        return kp[kpidx].btime[bidx].sec;
}
/*------------------------------------------------------------------------------
	GetDataBlockObsoleteTime()
	--------------------------
	Gets block data obsolete time (UNIX time, GMT since January 1, 1970).
        Parameters:
        	DeviceSystemNo  - 	device system number, IN
	        BlockNo         - 	device block number, IN
	        time            - 	pointer to time buffer, OUT
	Returns:
        	Function ret. code.
------------------------------------------------------------------------------*/
int GetDataBlockObsoleteTime(int kpn,int bn,OUT DWORD*time)
{
int kpidx,bidx;

        if(!GetSrcStatus())			return DATASRC_ERROR;
        kpidx=GetDevHandleBySno(kpn);
        if(kpidx<0)				return DATASRC_DEV_ERROR;
        bidx=GetBlockHandle(kpidx,bn);
        if(bidx<0)				return DATASRC_BLOCK_ERROR;
        if(osIsBadWritePtr(time,sizeof(*time)))   return OUTBUF_ERROR;

        *time=kp[kpidx].bpactime[bidx]+kp[kpidx].tout;

return DATASRC_OK;
}
/*------------------------------------------------------------------------------
	GetDataBlockBaseTime()
	----------------------
	Gets block data update timeout (UNIX time, GMT since January 1, 1970).
        Parameters:
        	DeviceSystemNo  - 	device system number, IN
	        BlockNo         - 	device block number, IN
	        time            - 	pointer to time buffer, OUT
	Returns:
        	Function ret. code.
------------------------------------------------------------------------------*/
int GetDataBlockBaseTime(int kpn,int bn,OUT DWORD*time)
{
int kpidx,bidx;

        if(!GetSrcStatus())		        return DATASRC_ERROR;
        kpidx=GetDevHandleBySno(kpn);
        if(kpidx<0)				return DATASRC_DEV_ERROR;
        bidx=GetBlockHandle(kpidx,bn);
        if(bidx<0)				return DATASRC_BLOCK_ERROR;
        if(osIsBadWritePtr(time,sizeof(*time)))   return OUTBUF_ERROR;

        *time=kp[kpidx].treq;

return DATASRC_OK;
}
/*------------------------------------------------------------------------------
	IsDataBlockValid()
        Gets device data block validity status.
        Parameters:
                DeviceSystemNo	- system device number
                BlockNo		- device data block number
        Returns: TRUE 	- device data block data is valid
        	 FALSE 	- device data block data is obsolete
------------------------------------------------------------------------------*/
BOOL IsDataBlockValid(int kpn,int bn)
{
int kpidx,bidx;
time_t t;

        if(!GetSrcStatus())		        return FALSE;
        kpidx=GetDevHandleBySno(kpn);
        if(kpidx<0)				return FALSE;
        bidx=GetBlockHandle(kpidx,bn);
        if(bidx<0)				return FALSE;

        time(&t);
        if((kp[kpidx].bpactime[bidx]+kp[kpidx].tout) >= (unsigned)t)return TRUE;

return FALSE;
}
/*------------------------------------------------------------------------------
	IsDataBlockValidEx()
        Gets device data block validity status.
        Parameters:
                DeviceSystemNo	- system device number
                BlockNo		- device data block number
        Returns: TRUE 	- device data block data is valid
        	 FALSE 	- device data block data is obsolete
------------------------------------------------------------------------------*/
BOOL IsDataBlockValidEx(int kpidx,int bidx)
{
time_t t;

        if(!GetSrcStatus())		        return FALSE;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))	return FALSE;
        if((bidx<0) || (bidx>=kp[kpidx].nboards)) return FALSE;

        time(&t);
        if((kp[kpidx].bpactime[bidx]+kp[kpidx].tout) >= (unsigned)t)return TRUE;

return FALSE;
}
/*------------------------------------------------------------------------------
	IsDeviceDataValid()
        Gets device data block validity status.
        Parameters:
                hDevice	- device handle
                hBlock  - device data block handle, if hBlock<0 any block
        Returns: TRUE 	- device data block data is valid
        	 FALSE 	- device data block data is obsolete
------------------------------------------------------------------------------*/
BOOL IsDeviceDataValid(int kpidx,int bidx)
{
int i;
unsigned wt;
time_t t;
long dt;

        if(!GetSrcStatus())   return FALSE;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return FALSE;
        time(&t);
        if(bidx<0)
        {
 	        wt=0;
         	for(i=0;i<kp[kpidx].nboards;i++)if(kp[kpidx].bpactime[i]>wt)wt=kp[kpidx].bpactime[i];
        }
        else
        {
 	        if(bidx>=kp[kpidx].nboards)return FALSE;
         	wt=kp[kpidx].bpactime[bidx];
        }
        dt=t-wt;
        if(dt<kp[kpidx].tout)	return TRUE;
        else 			return FALSE;
}
/*------------------------------------------------------------------------------
	GetDataBlock()
	--------------
	Gets block data
        Parameters:
        	DeviceSystemNo  - 	device system number,   IN
	        BlockNo         - 	device block number,    IN
	        time            - 	pointer to time of data recieving buffer, OUT
                btime           -       pointer to time of data measuring buffer, OUT
	        buf             - 	pointer to data buffer, OUT
	        num             - 	pointer to block data quantity, IN/OUT
	        type            -	pointer to data type,   OUT
	Returns:
        	Function ret. code.
------------------------------------------------------------------------------*/
int GetDataBlock(int kpn,int bn,OUT DWORD*time,OUT time_sms*btime,OUT DWORD*buf,IN OUT int*num,OUT BYTE*type)
{
        return GetDataBlockEx(kpn,bn,-1,time,btime,buf,num,type);
}
/*----------------------------------------------------------------------------*/
int GetDataBlockEx(int kpn,int bn,int btype,OUT DWORD*time,OUT time_sms*btime,OUT DWORD*buf,IN OUT int*num,OUT BYTE*type)
{
int kpidx;
        kpidx=GetDevHandleBySno(kpn);
        if(kpidx<0)				return DATASRC_DEV_ERROR;
        return GetDataBlockExByHandle(kpidx,bn,btype,time,btime,buf,num,type);
}
int GetDataBlockExByHandle(int kpidx,int bn,int btype,OUT DWORD*time,OUT time_sms*btime,OUT DWORD*buf,IN OUT int*num,OUT BYTE*type)
{
int i;
int inum;
int bidx;
BYTE typeM = 0; // by AVN

if(osIsBadWritePtr(num,sizeof(*num)))		return OUTBUF_ERROR;
inum=*num;
*num=0;
if(!GetSrcStatus())				return DATASRC_ERROR;
if((kpidx<0) || (kpidx>=SWH.KPtSize))           return DATASRC_DEV_ERROR;
bidx=GetBlockHandleExt(kpidx,bn,btype);
if(bidx<0)bidx=GetBlockHandle(kpidx,bn);
if(bidx<0)					return DATASRC_BLOCK_ERROR;
if(osIsBadWritePtr(time,sizeof(*time)))		return OUTBUF_ERROR;
if(osIsBadWritePtr(btime,sizeof(*btime)))		return OUTBUF_ERROR;
if(osIsBadWritePtr(buf,sizeof(DWORD)*(*num)))	return OUTBUF_ERROR;
if(osIsBadWritePtr(type,sizeof(*type)))		return OUTBUF_ERROR;

for(i=0;((i<inum)&&(i<kp[kpidx].bsnum[bidx]));i++) {
	buf[i]=tm[kp[kpidx].bfrom[bidx]+i].value.dwval;
    if (typeM < tm[kp[kpidx].bfrom[bidx]+i].vtype) typeM = tm[kp[kpidx].bfrom[bidx]+i].vtype; // by AVN
}

*time=kp[kpidx].bpactime[bidx];
btime->sec=kp[kpidx].btime[bidx].sec;
btime->msec=kp[kpidx].btime[bidx].msec;
*num=i;
//*type=tm[kp[kpidx].bfrom[bidx]].vtype;
*type=typeM;                                  // by AVN
return DATASRC_OK;
}
/*------------------------------------------------------------------------------
	SetDataBlock()
	--------------
	Sets block data
        Parameters:
        	DeviceSystemNo  - 	device system number,   IN
	        BlockNo         - 	device block number,    IN
                btime           -       pointer to time of data measuring buffer, IN
	        buf             - 	pointer to data buffer, IN
	        num             - 	pointer to block data quantity, IN/OUT
	        type            -	data type,              IN
	Returns:
        	Function ret. code.
------------------------------------------------------------------------------*/
int SetDataBlock(int kpn,int bn,IN time_sms*btime,IN DWORD*buf,IN OUT int*num,BYTE type)
{
        return SetDataBlockEx(kpn,bn,-1,btime,buf,num,type);
}
/*----------------------------------------------------------------------------*/
int SetDataBlockEx(int kpn,int bn,int btype,IN time_sms*btime,IN DWORD*buf,IN OUT int*num,BYTE type)
{
int i;
int inum;
int kpidx,bidx;
time_t pactime;

if(osIsBadWritePtr(num,sizeof(*num)))			return OUTBUF_ERROR;
inum=*num;
*num=0;
if(!GetSrcStatus())					return DATASRC_ERROR;
kpidx=GetDevHandleBySno(kpn);
if(kpidx<0)				   		return DATASRC_DEV_ERROR;
bidx=GetBlockHandleExt(kpidx,bn,btype);
if(bidx<0)bidx=GetBlockHandle(kpidx,bn);
if(bidx<0)				   		return DATASRC_BLOCK_ERROR;
if(osIsBadReadPtr(buf,sizeof(DWORD)*(*num)))
							return OUTBUF_ERROR;
if(osIsBadReadPtr(btime,sizeof(time_sms)))		return OUTBUF_ERROR;
if(	(type!= ID_BVAL) &&
        (type!= ID_CVAL) &&
	(type!= ID_SHVAL) &&
	(type!= ID_WVAL) &&
	(type!= ID_FVAL) &&
	(type!= ID_IVAL) &&
	(type!= ID_DWVAL) &&
	(type!= ID_TVAL))				return DATASRC_TYPE_ERROR;

for(i=0;((i<inum)&&(i<kp[kpidx].bsnum[bidx]));i++)
{
	tm[kp[kpidx].bfrom[bidx]+i].value.dwval=buf[i];
	tm[kp[kpidx].bfrom[bidx]+i].vtype=type;
	tm[kp[kpidx].bfrom[bidx]+i].flag=~0;
}
time(&pactime);
kp[kpidx].bpactime[bidx]=pactime;
kp[kpidx].btime[bidx].sec=btime->sec;
kp[kpidx].btime[bidx].msec=btime->msec;
if(!kp[kpidx].diag[0])kp[kpidx].KPf|=1;
kp[kpidx].diag[0]=1;
kp[kpidx].elapsedtime=0;
*num=i;
return DATASRC_OK;
}
/*------------------------------------------------------------------------------
 *	GetBlockChangeCount()
 *----------------------------------------------------------------------------*/
DWORD GetBlockChangeCount(int hDevice,int hBlock)
{
int i;
DWORD num=0;
        if(!GetSrcStatus())                             return 0;
        if((hDevice<0) || (hDevice>=SWH.KPtSize))       return 0;
        if((hBlock<0) || (hBlock>=kp[hDevice].nboards)) return 0;
        for(i=0;i<kp[hDevice].bsnum[hBlock];i++)if(tm[kp[hDevice].bfrom[hBlock]+i].flag)num++;
return num;
}
/*------------------------------------------------------------------------------
 *	SetBlockChangeFlags()
 *----------------------------------------------------------------------------*/
BOOL SetBlockChangeFlags(int hDevice,int hBlock)
{
int i;
        if(!GetSrcStatus())                             return FALSE;
        if((hDevice<0) || (hDevice>=SWH.KPtSize))       return FALSE;
        if((hBlock<0) || (hBlock>=kp[hDevice].nboards)) return FALSE;
        for(i=0;i<kp[hDevice].bsnum[hBlock];i++)tm[kp[hDevice].bfrom[hBlock]+i].flag=(~0);
        return TRUE;
}
/*------------------------------------------------------------------------------
 *	GetBlockFirstChangedTagNo()
 *----------------------------------------------------------------------------*/
int GetBlockFirstChangedTagNo(int hDevice,int hBlock)
{
int i,j;
BYTE fl;
        if(!GetSrcStatus())                             return -1;
        if((hDevice<0) || (hDevice>=SWH.KPtSize))       return -1;
        if((hBlock<0) || (hBlock>=kp[hDevice].nboards)) return -1;
        for(i=0;i<kp[hDevice].bsnum[hBlock];i++)
        {
          fl=tm[kp[hDevice].bfrom[hBlock]+i].flag;
          if(fl)
          {
                if( (kp[hDevice].btype[hBlock]!=TS) && (kp[hDevice].btype[hBlock]!=TU) )return i;
                for(j=0;j<8;j++)if(fl & (0x80>>j))return (i*8+j);
          }
        }
return -1;
}
/*------------------------------------------------------------------------------
 *	CopyBlock()
 *	------------------
 *	Copies data from source block to destination
 *      Parameters:
 *        	hDestDev - 	Destination device handle,   IN
 *        	hDestBlock - 	Destination block handle,    IN
 *        	hSrcDev - 	Source device handle,        IN
 *        	hSrcBlock - 	Source block handle,         IN
 *      Remarks:
 *                The Source & Destination block's types must be the same.
 *                Function does follows:
 *                - copies all source device block data to the destination block
 *                - sets service flags;
 *	Returns:
 *        	TRUE - success,
 *              FALSE - error - bad parameters, or the configuration is not the same,
 *                      or data is obsolete
 *
 *----------------------------------------------------------------------------*/
BOOL CopyBlock(int hDestDev,int hDestBlock,int hSrcDev,int hSrcBlock)
{
int i;
BYTE vtype;
BYTE v[100];
time_t t;

        if(!GetSrcStatus())return FALSE;
        if((hDestDev   < 0) || (hDestDev   >= SWH.KPtSize))             return FALSE;
        if((hSrcDev    < 0) || (hSrcDev    >= SWH.KPtSize))             return FALSE;
        if((hDestBlock < 0) || (hDestBlock >= kp[hDestDev].nboards))    return FALSE;
        if((hSrcBlock  < 0) || (hSrcBlock  >= kp[hSrcDev].nboards))     return FALSE;
        if(kp[hDestDev].btype[hDestBlock]!=kp[hSrcDev].btype[hSrcBlock])return FALSE;

        time(&t);
        if((unsigned)t > (kp[hSrcDev].bpactime[hSrcBlock]+kp[hSrcDev].tout))return FALSE;
        switch(kp[hSrcDev].btype[hSrcBlock])
        {
                default:
                for(i=0;(i<kp[hSrcDev].bsnum[hSrcBlock])&&(i<kp[hDestDev].bsnum[hDestBlock]);i++)
                        if(GetTmValue(hSrcDev,hSrcBlock,i,v,&vtype))
                                if(SetTmValue(hDestDev,hDestBlock,i,v,vtype,(kp[hDestDev].btype[hDestBlock]==TS)?FALSE:TRUE))
                {
                          tm[kp[hDestDev].bfrom[hDestBlock]+i].flag=tm[kp[hSrcDev].bfrom[hSrcBlock]+i].flag;
//                        tm[kp[hSrcDev].bfrom[hSrcBlock]+i].flag=0;
                }
                kp[hDestDev].bpactime[hDestBlock]   = kp[hSrcDev].bpactime[hSrcBlock];
                kp[hDestDev].btime[hDestBlock].sec  = kp[hSrcDev].btime[hSrcBlock].sec;
                kp[hDestDev].btime[hDestBlock].msec = kp[hSrcDev].btime[hSrcBlock].msec;
                        break;
                case TU:case TR:
                for(i=0;(i<kp[hSrcDev].bsnum[hSrcBlock])&&(i<kp[hDestDev].bsnum[hDestBlock]);i++)
                        if(GetTmValue(hSrcDev,hSrcBlock,i,v,&vtype))
                                if(SetTmValue(hDestDev,hDestBlock,i,v,vtype,(kp[hDestDev].btype[hDestBlock]==TU)?FALSE:TRUE))
                {
                        tm[kp[hDestDev].bfrom[hDestBlock]+i].flag=tm[kp[hSrcDev].bfrom[hSrcBlock]+i].flag;
//                        tm[kp[hSrcDev].bfrom[hSrcBlock]+i].flag=0;
                }
                kp[hDestDev].bpactime[hDestBlock]   = kp[hSrcDev].bpactime[hSrcBlock];
                kp[hDestDev].btime[hDestBlock].sec  = kp[hSrcDev].btime[hSrcBlock].sec;
                kp[hDestDev].btime[hDestBlock].msec = kp[hSrcDev].btime[hSrcBlock].msec;
                        break;
        }
return TRUE;
}
/*------------------------------------------------------------------------------
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
                        DIT API - TAG functions
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
*/
int GetTmRecordsQuantity(void)
{
        if(!GetSrcStatus())return 0;
        return SWH.TMtSize;
}
/*------------------------------------------------------------------------------
	GetTmValue()
	------------------
	Gets block's data point value
        Parameters:
        	hDevice - 	device handle,          IN
	        hBlock  - 	block handle,           IN
                no      -       block data point number,IN.
                v       -       pointer to value buffer,OUT
                vtype   -       value type,             OUT
	Returns:
        	TRUE - success, FALSE - error.
------------------------------------------------------------------------------*/
BOOL GetTmValue(int kpidx,int bidx,int n,void*v,unsigned char*vtype)
{
int off;

if(!GetSrcStatus())return FALSE;
if((kpidx<0) || (kpidx>=SWH.KPtSize))return FALSE;
if((bidx<0) || (bidx>=kp[kpidx].nboards))return FALSE;
if((n<0) || (n>=kp[kpidx].bsnum[bidx]))return FALSE;

off=kp[kpidx].bfrom[bidx]+n;
switch(tm[off].vtype)
{
case ID_BVAL:	*((BYTE*)v)=    tm[off].value.bval; 	break;
case ID_CVAL:	*((char*)v)=    tm[off].value.cval; 	break;
case ID_SHVAL:	*((SHORT*)v)=   tm[off].value.shval;    break;
case ID_WVAL:	*((WORD*)v)=    tm[off].value.wval;     break;
case ID_FVAL:	*((float*)v)=   tm[off].value.fval;     break;
case ID_IVAL:	*((int*)v)=     tm[off].value.ival;  	break;
case ID_DWVAL:	*((DWORD*)v)=   tm[off].value.dwval;	break;
default: return FALSE;
}
*vtype=(BYTE)tm[off].vtype;
return TRUE;
}
/*------------------------------------------------------------------------------
	SetTmValue()
	------------------
	Sets block's data point value
        Parameters:
        	hDevice - 	device handle,          IN
	        hBlock  - 	block handle,           IN
                no      -       block data point number,IN.
                v       -       pointer to value buffer,IN
                vtype   -       value type,             IN
                analog  -       if TRUE, sets block value as analog, IN
	Returns:
        	TRUE - success, FALSE - error.
------------------------------------------------------------------------------*/
BOOL SetTmValue(int kpidx,int bidx,int n,void*v,unsigned char vtype,BOOL analog)
{
int off;
DWORD dw;
int i;
float f;
SHORT sh;
WORD w;
BYTE b,bold;
char chv;

if(!GetSrcStatus())return FALSE;
if((kpidx<0) || (kpidx>=SWH.KPtSize))return FALSE;
if((bidx<0) || (bidx>=kp[kpidx].nboards))return FALSE;
if((n<0) || (n>=kp[kpidx].bsnum[bidx]))return FALSE;

off=kp[kpidx].bfrom[bidx]+n;
switch(vtype)
{
case ID_BVAL:   b=*((BYTE*)v);
                bold=tm[off].value.bval;
		if(b!=bold)
                {
                        tm[off].value.bval=b;	/*Сначала запись значения!!!-при работе по изменениям v2mstsr
                                                  сразу после установки флага выполняет чтение значения и сбрасывает флаг.
                                                  Если установить флаг до записи значения, v2mstsr вычитатет старое
                                                  значение и сбросит флаг до завершение записи нового значения,
                                                  и новое значение считано не будет, т.к. флаг изменения для него
                                                  уже сброшен!*/
			if(analog)	tm[off].flag=0xff;/*Потом установка флага-сигнал v2mstsr читать уже установленное значение*/
                	else 		tm[off].flag=b^bold;
                }
		break;
case ID_CVAL:   chv=*((char*)v);
                if(chv!=tm[off].value.cval)
                {
                        tm[off].value.cval=chv;
                        tm[off].flag=0xff;
                }
                break;
case ID_SHVAL:	sh=*((SHORT*)v);
                if(sh!=tm[off].value.shval)
                {
                        tm[off].value.shval=sh;
                        tm[off].flag=0xff;
                }
                break;
case ID_WVAL:	w=*((WORD*)v);
		if(w!=tm[off].value.wval)
                {
                        tm[off].value.wval=w;
                        tm[off].flag=0xff;
                }
                break;
case ID_FVAL:	f=*((float*)v);
		if(f!=tm[off].value.fval)
                {
                        tm[off].value.fval=f;
                        tm[off].flag=0xff;
                }
		break;
case ID_IVAL:	i=*((int*)v);
		if(i!=tm[off].value.ival)
                {
                        tm[off].value.ival=i;	
                        tm[off].flag=0xff;
                }
		break;
case ID_DWVAL:	dw=*((DWORD*)v);
		if(dw!=tm[off].value.dwval)
                {
                        tm[off].value.dwval=dw;	
                        tm[off].flag=0xff;
                }
		break;
default: return FALSE;
}
tm[off].vtype=vtype;
return TRUE;
}
/*------------------------------------------------------------------------------
	GetByteTmValue()
	------------------
	Gets block's data point value
        Parameters:
        	hDevice - 	device handle,          IN
	        hBlock  - 	block handle,           IN
                no      -       block data point number,IN.
	Returns:
        	BYTE value.
------------------------------------------------------------------------------*/
BYTE GetByteTmValue(int kpidx,int bidx,int n)
{
int off;
BYTE b;

if(!GetSrcStatus())return 0;
if((kpidx<0) || (kpidx>=SWH.KPtSize))return 0;
if((bidx<0) || (bidx>=kp[kpidx].nboards))return 0;
if((n<0) || (n>=kp[kpidx].bsnum[bidx]))return 0;

off=kp[kpidx].bfrom[bidx]+n;
switch(tm[off].vtype)
{
case ID_BVAL:	b=(BYTE)tm[off].value.bval;break;
case ID_CVAL:	b=(BYTE)tm[off].value.cval;break;
case ID_SHVAL:	b=(BYTE)tm[off].value.shval;break;
case ID_WVAL:	b=(BYTE)tm[off].value.wval;break;
case ID_FVAL:	b=(BYTE)(tm[off].value.fval+0.5);break;
case ID_IVAL:	b=(BYTE)tm[off].value.ival;break;
case ID_DWVAL:	b=(BYTE)tm[off].value.dwval;break;
}
return b;
}
/*------------------------------------------------------------------------------
 *	GetWORDTmValue()
 *----------------------------------------------------------------------------*/
WORD GetWORDTmValue(int kpidx,int bidx,int n)
{
int off;
WORD w;

if(!GetSrcStatus())return 0;
if((kpidx<0) || (kpidx>=SWH.KPtSize))return 0;
if((bidx<0) || (bidx>=kp[kpidx].nboards))return 0;
if((n<0) || (n>=kp[kpidx].bsnum[bidx]))return 0;

off=kp[kpidx].bfrom[bidx]+n;
switch(tm[off].vtype)
{
case ID_BVAL:	w=(WORD)tm[off].value.bval;break;
case ID_CVAL:	w=(WORD)tm[off].value.cval;break;
case ID_SHVAL:	w=(WORD)tm[off].value.shval;break;
case ID_WVAL:	w=(WORD)tm[off].value.wval;break;
case ID_FVAL:	w=(WORD)(tm[off].value.fval+0.5);break;
case ID_IVAL:	w=(WORD)tm[off].value.ival;break;
case ID_DWVAL:	w=(WORD)tm[off].value.dwval;break;
}
return w;
}
/*------------------------------------------------------------------------------
 *	GetDWORDTmValue()
 *----------------------------------------------------------------------------*/
DWORD GetDWORDTmValue(int kpidx,int bidx,int n)
{
int off;
DWORD dw;

if(!GetSrcStatus())return 0;
if((kpidx<0) || (kpidx>=SWH.KPtSize))return 0;
if((bidx<0) || (bidx>=kp[kpidx].nboards))return 0;
if((n<0) || (n>=kp[kpidx].bsnum[bidx]))return 0;

off=kp[kpidx].bfrom[bidx]+n;
switch(tm[off].vtype)
{
case ID_BVAL:	dw=(DWORD)tm[off].value.bval;break;
case ID_CVAL:	dw=(DWORD)tm[off].value.cval;break;
case ID_SHVAL:	dw=(DWORD)tm[off].value.shval;break;
case ID_WVAL:	dw=(DWORD)tm[off].value.wval;break;
case ID_FVAL:	dw=(DWORD)tm[off].value.fval;break;
case ID_IVAL:	dw=(DWORD)tm[off].value.ival;break;
case ID_DWVAL:	dw=(DWORD)tm[off].value.dwval;break;
}
return dw;
}
/*------------------------------------------------------------------------------
 *	GetFloatTmValue()
 *----------------------------------------------------------------------------*/
float GetFloatTmValue(int kpidx,int bidx,int n)
{
int off;
float f;

if(!GetSrcStatus())return 0;
if((kpidx<0) || (kpidx>=SWH.KPtSize))return 0;
if((bidx<0) || (bidx>=kp[kpidx].nboards))return 0;
if((n<0) || (n>=kp[kpidx].bsnum[bidx]))return 0;

off=kp[kpidx].bfrom[bidx]+n;
switch(tm[off].vtype)
{
case ID_BVAL:	f=(float)tm[off].value.bval;break;
case ID_CVAL:	f=(float)tm[off].value.cval;break;
case ID_SHVAL:	f=(float)tm[off].value.shval;break;
case ID_WVAL:	f=(float)tm[off].value.wval;break;
case ID_FVAL:	f=(float)tm[off].value.fval;break;
case ID_IVAL:	f=(float)tm[off].value.ival;break;
case ID_DWVAL:	f=(float)tm[off].value.dwval;break;
}
return f;
}
/*------------------------------------------------------------------------------
 *	GetIntTmValue()
 *----------------------------------------------------------------------------*/
int GetIntTmValue(int kpidx,int bidx,int n)
{
int off;
int di;

if(!GetSrcStatus())return 0;
if((kpidx<0) || (kpidx>=SWH.KPtSize))return 0;
if((bidx<0) || (bidx>=kp[kpidx].nboards))return 0;
if((n<0) || (n>=kp[kpidx].bsnum[bidx]))return 0;

off=kp[kpidx].bfrom[bidx]+n;
switch(tm[off].vtype)
{
case ID_BVAL:	di=(int)tm[off].value.bval;break;
case ID_CVAL:	di=(int)tm[off].value.cval;break;
case ID_SHVAL:	di=(int)tm[off].value.shval;break;
case ID_WVAL:	di=(int)tm[off].value.wval;break;
case ID_FVAL:	di=(int)tm[off].value.fval;break;
case ID_IVAL:	di=(int)tm[off].value.ival;break;
case ID_DWVAL:	di=(int)tm[off].value.dwval;break;
}
return di;
}
/*------------------------------------------------------------------------------
 *	GetShortTmValue()
 *----------------------------------------------------------------------------*/
short GetShortTmValue(int kpidx,int bidx,int n)
{
int off;
short ds;

if(!GetSrcStatus())return 0;
if((kpidx<0) || (kpidx>=SWH.KPtSize))return 0;
if((bidx<0) || (bidx>=kp[kpidx].nboards))return 0;
if((n<0) || (n>=kp[kpidx].bsnum[bidx]))return 0;

off=kp[kpidx].bfrom[bidx]+n;
switch(tm[off].vtype)
{
case ID_BVAL:	ds=(short)tm[off].value.bval;break;
case ID_CVAL:	ds=(short)tm[off].value.cval;break;
case ID_SHVAL:	ds=(short)tm[off].value.shval;break;
case ID_WVAL:	ds=(short)tm[off].value.wval;break;
case ID_FVAL:	ds=(short)tm[off].value.fval;break;
case ID_IVAL:	ds=(short)tm[off].value.ival;break;
case ID_DWVAL:	ds=(short)tm[off].value.dwval;break;
}
return ds;
}
/*------------------------------------------------------------------------------
	GetTmValueType()
	---------------
	Gets block's data point value type
        Parameters:
        	hDevice - 	device handle,          IN
	        hBlock  - 	block handle,           IN
                no      -       block data point number,IN.
	Returns:
        	data point value type.
------------------------------------------------------------------------------*/
BYTE GetTmValueType(int kpidx,int bidx,int n)
{
        if(!GetSrcStatus())return 0;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return 0;
        if((bidx<0) || (bidx>=kp[kpidx].nboards))return 0;
        if((n<0) || (n>=kp[kpidx].bsnum[bidx]))return 0;
	return tm[kp[kpidx].bfrom[bidx]+n].vtype;
}
/*------------------------------------------------------------------------------
 *	GetMaxBlockValue()
 *----------------------------------------------------------------------------*/
float GetMaxBlockValue(int kpidx,int bidx)
{
int i;
float df,df1;

if(!GetSrcStatus())return 0;
if((kpidx<0) || (kpidx>=SWH.KPtSize))return 0;
if((bidx<0) || (bidx>=kp[kpidx].nboards))return 0;
df=GetFloatTmValue(kpidx,bidx,0);
for(i=0;i<kp[kpidx].bsnum[bidx];i++)
{
	df1=GetFloatTmValue(kpidx,bidx,i);
        if(df1>df)df=df1;
}
return df;
}
/*------------------------------------------------------------------------------
 *	GetAbsMaxBlockValue()
 *----------------------------------------------------------------------------*/
float GetAbsMaxBlockValue(int kpidx,int bidx)
{
int i;
float df,df1;

if(!GetSrcStatus())return 0;
if((kpidx<0) || (kpidx>=SWH.KPtSize))return 0;
if((bidx<0) || (bidx>=kp[kpidx].nboards))return 0;
df=GetFloatTmValue(kpidx,bidx,0);
for(i=0;i<kp[kpidx].bsnum[bidx];i++)
{
	df1=GetFloatTmValue(kpidx,bidx,i);
        if(df1<0)df1=df1*(-1);
        if(df1>df)df=df1;
}
return df;
}
/*------------------------------------------------------------------------------
 *	IsTmValueChanged()
 *----------------------------------------------------------------------------*/
BOOL IsTmValueChanged(int kpidx,int bidx,int n)
{
        if(!GetSrcStatus())return 0;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return 0;
        if((bidx<0) || (bidx>=kp[kpidx].nboards))return 0;
        if((n<0) || (n>=kp[kpidx].bsnum[bidx]))return 0;
        if(tm[kp[kpidx].bfrom[bidx]+n].flag)return TRUE;
        return FALSE;
}
/*------------------------------------------------------------------------------
 *	ResetTmValueChangeFlag()
 *----------------------------------------------------------------------------*/
BOOL ResetTmValueChangeFlag(int kpidx,int bidx,int n)
{
int off;

if(!GetSrcStatus())return FALSE;
if((kpidx<0) || (kpidx>=SWH.KPtSize))return FALSE;
if((bidx<0) || (bidx>=kp[kpidx].nboards))return FALSE;
if((n<0) || (n>=kp[kpidx].bsnum[bidx]))return FALSE;

off=kp[kpidx].bfrom[bidx]+n;
tm[off].flag=0;
return TRUE;
}
/*------------------------------------------------------------------------------
 *	SetTmValueChangeFlag()
 *----------------------------------------------------------------------------*/
BOOL SetTmValueChangeFlag(int kpidx,int bidx,int n)
{
        if(!GetSrcStatus())return 0;
        if((kpidx<0) || (kpidx>=SWH.KPtSize))return 0;
        if((bidx<0) || (bidx>=kp[kpidx].nboards))return 0;
        if((n<0) || (n>=kp[kpidx].bsnum[bidx]))return 0;
        tm[kp[kpidx].bfrom[bidx]+n].flag=(~0);
        return TRUE;
}
/*------------------------------------------------------------------------------
	GetTagD()
	---------------
	Gets block's data bit
        Parameters:
        	hDevice - 	device handle,          IN
	        hBlock  - 	block handle,           IN
                bitno   -       block data bit number,IN.
	Returns:
        	0 or 1
                -1 if error
------------------------------------------------------------------------------*/
int GetTagD(int hDev,int hBlock,int bitno)
{
int off;
BYTE mask;

if(!GetSrcStatus())                             return -1;
if((hDev<0) || (hDev>=SWH.KPtSize))             return -1;
if((hBlock<0) || (hBlock>=kp[hDev].nboards))    return -1;
if((kp[hDev].btype[hBlock]!=TS) &&
   (kp[hDev].btype[hBlock]!=TU))                return -1;
if((bitno<0)||(bitno/8>=kp[hDev].bsnum[hBlock]))return -1;

off=kp[hDev].bfrom[hBlock]+bitno/8;
mask=(BYTE)(0x80>>(bitno%8));
return (int)((tm[off].value.bval & mask)?1:0);
}
/*------------------------------------------------------------------------------
	SetTagD()
	---------------
	Sets block's data bit On or OFF
        Parameters:
        	hDevice - 	device handle,          IN
	        hBlock  - 	block handle,           IN
                bitno   -       block data bit number,IN.
                bitval  -       new bit value, IN
        Remarks:
                Set data bit of DI/DO block and set change flag if value was changed
	Returns:
        	TRUE - success
                FALSE - error
------------------------------------------------------------------------------*/
BOOL    SetTagD(int hDev,int hBlock,int bitno,BYTE bitval)
{
int off;
BYTE mask,oldval;

if(!GetSrcStatus())                             return FALSE;
if((hDev<0) || (hDev>=SWH.KPtSize))             return FALSE;
if((hBlock<0) || (hBlock>=kp[hDev].nboards))    return FALSE;
if((kp[hDev].btype[hBlock]!=TS) &&
   (kp[hDev].btype[hBlock]!=TU))                return FALSE;
if((bitno<0)||(bitno/8>=kp[hDev].bsnum[hBlock]))return FALSE;

off=kp[hDev].bfrom[hBlock]+bitno/8;
mask=(BYTE)(0x80>>(bitno%8));
oldval=(BYTE)((tm[off].value.bval & mask)?1:0);
bitval=(BYTE)(bitval?1:0);
if(bitval)        tm[off].value.bval|=mask;
else              tm[off].value.bval&=((BYTE)(~mask));
tm[off].vtype=ID_BVAL;
if(oldval!=bitval)tm[off].flag|=mask;
return TRUE;
}
/*------------------------------------------------------------------------------
	ForceTagD()
	---------------
	Force block's data bit On or OFF
        Parameters:
        	hDevice - 	device handle,          IN
	        hBlock  - 	block handle,           IN
                bitno   -       block data bit number,IN.
                bitval  -       new bit value, IN
        Remarks:
                Set data bit of DI/DO block and force bit change flag
	Returns:
        	TRUE - success
                FALSE - error
------------------------------------------------------------------------------*/
BOOL    ForceTagD(int hDev,int hBlock,int bitno,BYTE bitval)
{
int off;
BYTE mask;

if(!GetSrcStatus())                             return FALSE;
if((hDev<0) || (hDev>=SWH.KPtSize))             return FALSE;
if((hBlock<0) || (hBlock>=kp[hDev].nboards))    return FALSE;
if((kp[hDev].btype[hBlock]!=TS) &&
   (kp[hDev].btype[hBlock]!=TU))                return FALSE;
if((bitno<0)||(bitno/8>=kp[hDev].bsnum[hBlock]))return FALSE;
off=kp[hDev].bfrom[hBlock]+bitno/8;
mask=(BYTE)(0x80>>(bitno%8));
if(bitval)        tm[off].value.bval|=mask;
else              tm[off].value.bval&=((BYTE)(~mask));
tm[off].flag|=mask;
tm[off].vtype=ID_BVAL;
return TRUE;
}
/*------------------------------------------------------------------------------
	ResetTagDChangeFlag()
	---------------
	Resets block's data bit change flag
        Parameters:
        	hDevice - 	device handle,          IN
	        hBlock  - 	block handle,           IN
                bitno   -       block data bit number,IN.
                bitval  -       new bit value, IN
	Returns:
        	TRUE - success
                FALSE - error
------------------------------------------------------------------------------*/
BOOL    ResetTagDChangeFlag(int hDev,int hBlock,int bitno)
{
int off;
BYTE mask;

if(!GetSrcStatus())                             return FALSE;
if((hDev<0) || (hDev>=SWH.KPtSize))             return FALSE;
if((hBlock<0) || (hBlock>=kp[hDev].nboards))    return FALSE;
if((kp[hDev].btype[hBlock]!=TS) &&
   (kp[hDev].btype[hBlock]!=TU))                return FALSE;
if((bitno<0)||(bitno/8>=kp[hDev].bsnum[hBlock]))return FALSE;

off=kp[hDev].bfrom[hBlock]+bitno/8;
mask=(BYTE)(0x80>>(bitno%8));
tm[off].flag&=(BYTE)(~mask);
return TRUE;
}
BOOL    IsTagDChanged(int hDev,int hBlock,int bitno)
{
int off;
BYTE mask;

if(!GetSrcStatus())                             return FALSE;
if((hDev<0) || (hDev>=SWH.KPtSize))             return FALSE;
if((hBlock<0) || (hBlock>=kp[hDev].nboards))    return FALSE;
if((kp[hDev].btype[hBlock]!=TS) &&
   (kp[hDev].btype[hBlock]!=TU))                return FALSE;
if((bitno<0)||(bitno/8>=kp[hDev].bsnum[hBlock]))return FALSE;

off=kp[hDev].bfrom[hBlock]+bitno/8;
mask=(BYTE)(0x80>>(bitno%8));
if(tm[off].flag & mask)return TRUE;
return FALSE;
}
/*------------------------------------------------------------------------------
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
                        DIT API - SERVICE functions
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
*/

/*------------------------------------------------------------------------------
 *	GetRedundantDevsList()
 *	---------------------
 *      Parameters:
 *             MDevsList - pointer to redundant device handles,OUT;
 *             RDevsList - pointer to redundant device handles,OUT;
 *             listsz - lists size, IN
 *	Returns:
 *        	Number of redundant Devices.
 *----------------------------------------------------------------------------*/
int GetRedundantDevsList(int*MDevsList,int*RDevsList,int listsz)
{
int i,j,n=0;

for(i=0;i<SWH.KPtSize;i++)
        for(j=i+1;j<SWH.KPtSize;j++)if(kp[i].skpnum==kp[j].skpnum)
{
        if(n<listsz){   MDevsList[n]=i; RDevsList[n]=j;}
        n++;
        break;
}
return n;
}
/*------------------------------------------------------------------------------
 *	SyncRedundantDevs()
 *	------------------
 *	Synchronizes main and redundant device data
 *      Parameters:
 *        	hMDevice - 	Main device handle,         IN
 *	        hRDevice - 	Redundant device handle,    IN
 *      Remarks:
 *                The Main and Redundant device configuration must be the same!
 *                Function does follows:
 *                - copies all Redundant device input blocks data to the same type blocks of
 *                Main device;
 *                - copies all Main device output blocks data to the same type blocks of
 *                Redundant device;
 *                - sets service flags;
 *	Returns:
 *        	TRUE - success, FALSE - error - the configuration is not the same.
 *----------------------------------------------------------------------------*/
BOOL SyncRedundantDevs(int hMDevice,int hRDevice)
{
int i,j;
BYTE vtype;
BYTE v[100];
BYTE processed=0;
time_t t;
//----------------------------------Verifying parameters
        if(!GetSrcStatus())                                     return FALSE;
        if((hMDevice<0) || (hMDevice>=SWH.KPtSize))             return FALSE;
        if((hRDevice<0) || (hRDevice>=SWH.KPtSize))             return FALSE;
        if(kp[hMDevice].skpnum!=kp[hRDevice].skpnum)            return FALSE;
        if( (kp[hMDevice].type>100) || (kp[hRDevice].type>100)) return FALSE;
//----------------------------------Testing time-out conditions
        if(kp[hMDevice].elapsedtime < (unsigned)kp[hMDevice].tout)
        {
OffSync:        if(kp[hRDevice].diag[0]&1)      i=2;
                else                            i=0;
                if(kp[hMDevice].diag[10] != i)
                {
                        kp[hMDevice].diag[10] = i;
                        kp[hMDevice].KPf|=1;
                }
return FALSE;
        }
//----------------------------------Verifying parameters
//        if(!(kp[hRDevice].diag[0]&1))return FALSE;
        if((!kp[hMDevice].nboards)||(!kp[hRDevice].nboards))    return FALSE;
        for(i=0;(i<kp[hMDevice].nboards) && (i<kp[hRDevice].nboards);i++)
                if(kp[hMDevice].btype[i]!=kp[hRDevice].btype[i])return FALSE;
//----------------------------------Do redundant switching
        time(&t);
        for(i=0;(i<kp[hMDevice].nboards) && (i<kp[hRDevice].nboards);i++)
        	if((kp[hRDevice].bpactime[i]+kp[hRDevice].tout) >= (unsigned)t)
        {
                processed=1;
                switch(kp[hMDevice].btype[i])
                {
                default:
                for(j=0;(j<kp[hMDevice].bsnum[i])&&(j<kp[hRDevice].bsnum[i]);j++)
                        if(GetTmValue(hRDevice,i,j,v,&vtype))
                                if(SetTmValue(hMDevice,i,j,v,vtype,(kp[hMDevice].btype[i]==TS)?FALSE:TRUE))
                {
                        tm[kp[hRDevice].bfrom[i]+j].flag=0;
                }
                kp[hMDevice].bpactime[i]=kp[hRDevice].bpactime[i];
                kp[hMDevice].btime[i].sec=kp[hRDevice].btime[i].sec;
                kp[hMDevice].btime[i].msec=kp[hRDevice].btime[i].msec;
                        break;
                case TU:case TR:
                for(j=0;(j<kp[hMDevice].bsnum[i])&&(j<kp[hRDevice].bsnum[i]);j++)
                        if(GetTmValue(hMDevice,i,j,v,&vtype))
                                if(SetTmValue(hRDevice,i,j,v,vtype,(kp[hRDevice].btype[i]==TU)?FALSE:TRUE))
                {
                        tm[kp[hRDevice].bfrom[i]+j].flag=tm[kp[hMDevice].bfrom[i]+j].flag;
                        tm[kp[hMDevice].bfrom[i]+j].flag=0;
                }
                kp[hRDevice].bpactime[i]=kp[hMDevice].bpactime[i];
                kp[hRDevice].btime[i].sec=kp[hMDevice].btime[i].sec;
                kp[hRDevice].btime[i].msec=kp[hMDevice].btime[i].msec;
                        break;
                }
        }
        if(!processed)goto OffSync;
        for (i=0;i<8;i++)if(kp[hMDevice].diag[i]!=kp[hRDevice].diag[i])
        {
                kp[hMDevice].diag[i]=kp[hRDevice].diag[i];
                kp[hMDevice].KPf|=1;
        }
//        kp[hRDevice].diag[9]=kp[hMDevice].diag[9];
        if(kp[hRDevice].diag[0]&1)      i=3;
        else                            i=1;
        if(kp[hMDevice].diag[10] != i)
        {
                kp[hMDevice].diag[10]=i;
                kp[hMDevice].KPf|=1;
        }
return TRUE;
}

