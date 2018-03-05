#ifndef V2MDAPI
#define V2MDAPI
#include "os_defs.h"
/*------------------------------------------------------------------------------
	vpr
        09.1998

        		v2m DIT API library

------------------------------------------------------------------------------*/
/* 	GetDataBlock(),SetDataBlock(),GetDataBlockObsoleteTime() ret. codes*/
#define DATASRC_OK			0
#define DATASRC_ERROR			1
#define DATASRC_DEV_ERROR		2
#define DATASRC_BLOCK_ERROR		3
#define DATASRC_TYPE_ERROR		4
#define OUTBUF_ERROR			5
/*	Measured values types*/
#define ID_BVAL		0		/* 8 bit unsigned*/
#define ID_SHVAL	1		/* 16 bit signed*/
#define ID_WVAL		2		/* 16 bit unsigned*/
#define ID_FVAL		3		/* 32 bit IEEE float*/
#define ID_IVAL		4		/* 32 bit signed*/
#define ID_DWVAL	5		/* 32 bit unsigned*/
#define ID_TVAL		6		/* 32 bit time*/
#define ID_CVAL		100		/* 8 bit unsigned*/
/*	v2m time definition structer*/
typedef struct
{
	DWORD	sec;
	WORD msec;
}time_sms;
/*------------------------------------------------------------------------------

			Library functions: Common

------------------------------------------------------------------------------*/
extern "C"
{
BOOL AttachDataSource(void);
int AttachDataSourceEx(char*fname,DWORD flags);
void DetachDataSource(void);
BOOL GetSrcStatus(void);
BOOL SaveDITToFile(char*fname);
BOOL RestoreDITFromFile(char*fname);
WORD GetFlags(void);
WORD GetSysNo(void);
BOOL SetDebugMode(BYTE mode);
BOOL IsDebugging(void);
BOOL IsLogging();
/*------------------------------------------------------------------------------

			Library functions: PORT

------------------------------------------------------------------------------*/
WORD GetPortsQuantity(void);
int GetPortHandle(int PortNo);
int GetPortNo(int hPort);
BYTE GetPortSpeedIdx(int hPort);
BYTE GetPortUARTParams(int hPort);
BYTE GetPortFlags(int hPort);
BYTE GetPortType(int hPort);
BYTE GetPortProto(int hPort);
unsigned GetPortTout(int hPort);
unsigned GetPortConWaitTime(int hPort);
unsigned GetPortElapsedTime(int hPort);
unsigned IncPortElapsedTime(int hPort);
unsigned IncPortStat(int hPort,int no);
unsigned GetPortStat(int hPort,int no);
BOOL SetPortStat(int pidx,int sn,unsigned dwValue);
BOOL ResetPortStat(int hPort,int no);
BOOL ResetPortElapsedTime(int pidx);
BOOL IsPortOn(int hPort);
BOOL SetPortOn(int hPort);
BOOL SetPortOff(int hPort);
BOOL SetPortOnDebug(int hPort,BYTE mode);       /* mode=1/0 - ON/OFF*/
BOOL IsPortOnDebug(int hPort);
BOOL SetPortsOnDebug(BYTE mode);
int GetFirstPortDevHandle(int hPort);
int GetFirstPortDevType(int hPort);
int GetPortDevsNum(int hPort);
BOOL GetPortInitString(int hPort,int no,char*strbuf);
/*------------------------------------------------------------------------------

			Library functions: DEVICE

------------------------------------------------------------------------------*/
WORD GetDevsQuantity(void);
int GetDevHandleBySno(int DeviceSystemNo);
int GetRDevHandleBySno(int skpnum);
int GetDevHandleByNo(int hPort,int DeviceNo,int DeviceType);
int GetDevSno(int hDevice);
int GetDevNo(int hDevice);
int GetDevType(int hDevice);
int GetDevPortHandle(int hDevice);
int GetDevPortNo(int hDevice);
int GetDevBlocksNum(int hDevice);
unsigned GetDevSessionTout(int hDevice);
unsigned GetDevPollTime(int hDevice);
unsigned GetDevPollWindowTime(int hDevice);
WORD GetDevFlags(int hDevice);
WORD SetDevFlags(int kpidx,WORD fl);
BYTE GetDevDiag(int hDevice,int no);
BYTE SetDevDiag(int kpidx,int no,BYTE newval);
unsigned GetDevElapsedTime(int hDevice);
unsigned IncDevElapsedTime(int hDevice);
unsigned GetDevStat(int hDevice,int no);
unsigned SetDevStat(int kpidx,int sn,unsigned stat);
unsigned IncDevStat(int hDevice,int no);
unsigned DecDevStat(int hDevice,int no);
BOOL ResetDevStat(int hDevice,int no);
BOOL ResetDevStats(int kpidx);
BOOL ResetDevElapsedTime(int kpidx);
BOOL IsDevOnLine(int hDevice);
BOOL IsDevHealthy(int hDevice);
BOOL IsDevOnPoll(int hDevice);
BOOL IsDevOnAck(int hDevice);
BOOL IsDevOnControl(int hDevice);
BOOL SetDevOnPoll(int hDevice);
BOOL SetDevOnAck(int hDevice);
BOOL SetDevOnControl(int hDevice);
BOOL SetDevOnDebug(int hDevice,BYTE mode);        /* mode=1/0 - ON/OFF*/
BOOL IsDevOnDebug(int hDevice);
BOOL SetDevsOnDebug(BYTE mode);        		  /* mode=1/0 - ON/OFF*/
BOOL SetDevLinkStatus(int hDevice,BYTE mode);
BOOL GetDevCallStr(int hDevice,char*str);
BOOL SetDevChangeFlags(int hDevice);
BOOL ResetDevLatchedCommands(int hDevice);
/*------------------------------------------------------------------------------

			Library functions: BLOCK

------------------------------------------------------------------------------*/
int GetBlockHandle(int hDevice,int BlockNo);
int GetBlockHandleExt(int kpidx,int bnum,int type);
int GetBlockNo(int hDevice,int hBlock);
int GetBtype(int hDevice,int hBlock);
int GetBnum(int hDevice,int hBlock);
int GetBsnum(int hDevice,int hBlock);
int GetBlockTagsNum(int kpidx,int bidx);
int GetBflags(int hDevice,int hBlock);
DWORD GetBpactime(int hDevice,int hBlock);
DWORD SetBpactime(int kpidx,int bidx,DWORD t);
DWORD GetBtime(int hDevice,int hBlock,time_sms*tm);
DWORD SetBtime(int kpidx,int bidx,time_sms*tm);
int GetDataBlockObsoleteTime(int DeviceSystemNo,int BlockNo,DWORD*time);
int GetDataBlockBaseTime(int DeviceSystemNo,int BlockNo,DWORD*time);
BOOL IsDataBlockValid(int DeviceSystemNo,int BlockNo);
BOOL IsDataBlockValidEx(int hDev,int hBlock);
BOOL IsDeviceDataValid(int hDevice,int hBlock);
int GetDataBlock(int DeviceSystemNo,int BlockNo,DWORD*time,OUT time_sms*btime,OUT DWORD*buf,IN OUT int*num,OUT BYTE*type);
int GetDataBlockEx(int DeviceSystemNo,int BlockNo,int BlockType,DWORD*time,OUT time_sms*btime,OUT DWORD*buf,IN OUT int*num,OUT BYTE*type);
int GetDataBlockExByHandle(int hDevice,int BlockNo,int BlockType,DWORD*time,OUT time_sms*btime,OUT DWORD*buf,IN OUT int*num,OUT BYTE*type);
int SetDataBlock(int DeviceSystemNo,int BlockNo,time_sms*btime,DWORD*buf,int*num,BYTE type);
int SetDataBlockEx(int DeviceSystemNo,int BlockNo,int BlockType,time_sms*btime,DWORD*buf,int*num,BYTE type);
DWORD   GetBlockChangeCount(int hDevice,int hBlock);
BOOL    SetBlockChangeFlags    (int hDevice,int hBlock);
int     GetBlockFirstChangedTagNo(int hDevice,int hBlock);
BOOL    CopyBlock(int hDestDev,int hDestBlock,int hSrcDev,int hSrcBlock);
/*------------------------------------------------------------------------------

			Library functions: TAG

------------------------------------------------------------------------------*/
int     GetTmRecordsQuantity(void);
BOOL    GetTmValue      (int hDevice,int hBlock,int no,void*v,unsigned char*vtype);
BOOL    SetTmValue      (int hDevice,int hBlock,int no,void*v,unsigned char vtype,BOOL analog);
BYTE    GetByteTmValue  (int hDevice,int hBlock,int no);
WORD    GetWORDTmValue  (int hDevice,int hBlock,int no);
DWORD   GetDWORDTmValue (int hDevice,int hBlock,int no);
float   GetFloatTmValue (int hDevice,int hBlock,int no);
int     GetIntTmValue   (int kpidx,int bidx,int n);
short   GetShortTmValue (int kpidx,int bidx,int n);
BYTE    GetTmValueType  (int hDevicex,int hBlock,int no);
float   GetMaxBlockValue(int hDevice,int hBlock);
float 	GetAbsMaxBlockValue(int hDevice,int hBlock);
BOOL    IsTmValueChanged(int hDevice,int hBlock,int no);
BOOL    ResetTmValueChangeFlag  (int hDevice,int hBlock,int no);
BOOL    SetTmValueChangeFlag    (int hDevice,int hBlock,int no);

int     GetTagD(int hDev,int hBlock,int bitno);
BOOL    SetTagD(int hDev,int hBlock,int bitno,BYTE bitval);
BOOL    ForceTagD(int hDev,int hBlock,int bitno,BYTE bitval);
BOOL    ResetTagDChangeFlag(int hDev,int hBlock,int bitno);
BOOL    IsTagDChanged(int hDev,int hBlock,int bitno);
/*------------------------------------------------------------------------------

			Library functions: SERVICE

------------------------------------------------------------------------------*/
int GetRedundantDevsList(int*MDevsList,int*RDevsList,int listsz);
BOOL SyncRedundantDevs(int hMDevice,int hRDevice);
}
#endif
