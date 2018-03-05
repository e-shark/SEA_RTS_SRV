#ifndef v2mConnectH
#define v2mConnectH

#include "Logger.h"
#include "prtSCUL.h"

//-----------------------------------------------------------------------------

typedef enum
{
    DI_KEY1, DI_KEY2, DI_KEY3, DI_KEY4,
    DI_PHASEA, DI_PHASEB, DI_PHASEC, DI_BATTERY,
    DI_INTR1, DI_INTR2, DI_ALARM1, DI_ALARM2,

    DI_1, DI_2, DI_3, DI_4,
    DI_5, DI_6, DI_7, DI_8,
    DI_9, DI_10, DI_11, DI_12,
    DI_13, DI_14, DI_15, DI_16,
    DI_SPLINE1, DI_SPLINE2, DI_SPLINE3, DI_SPLINE4
}eDIS;
//-----------------------------------------------------------------------------

void v2mSetLogger(TLogger *pLgg);
void v2mDynamicConnection(void);

DWORD SEAFlagsToRTUTS(DWORD SEAFlags);

bool v2mSetLiftState(WORD RTUaddr, DWORD RTUTS, DWORD state, DWORD &RTUTrF);
bool v2mCheckCmd(WORD RTUaddr, TPrtSCUL *SCULdrv);
bool v2mSetLinkStatus(WORD RTUaddr);

int GetRTUWithConnectTimeOut(int fromRTU, int threshold, int &OutTime);
void SetRTUTimer(int RTUNum, int TimerVal);
int GetRTUDevNo(int RTUNum);


#endif

