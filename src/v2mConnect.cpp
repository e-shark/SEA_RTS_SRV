
#include <sys/timeb.h>  

#include "v2mConnect.h"
#include "oswr.h"
#include "v2mdapi.h"
#include "kkpdevs.h"

TLogger *Logger = NULL;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void v2mSetLogger(TLogger *pLgg)
{
    if (pLgg) Logger = pLgg;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define SSDKKPIOMUTEXNAME	"Global\\SSD_kkpio"
BOOL IsKkpioRunning(void)
{
    HANDLE hBlockMutex;
    hBlockMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, SSDKKPIOMUTEXNAME);
    if (hBlockMutex == 0)return FALSE;
    CloseHandle(hBlockMutex);
    return TRUE;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void v2mDynamicConnection(void)
{
    //---------Do dynamic connection with v2m mmf-data source
     if (!IsKkpioRunning()) {
        if (GetSrcStatus()) {
            DetachDataSource();
            if (Logger) Logger->LogMessage("v2m dataSource detached", LOG_DBG1);
        }
    }
    else {
        if (!GetSrcStatus()) {
            if (AttachDataSource()) {
                if (Logger) Logger->LogMessage("v2m dataSource attached", LOG_DBG1);
            }
            else {
                if (Logger) Logger->LogMessage("Error: failed to attach v2m dataSource", LOG_DBG1);
            }
        }
    }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool RTUCommandProcessing(DWORD Cmd, DWORD Prm, TPrtSCUL *SCULdrv)
{
    bool res = false;
    if (!SCULdrv) return false;
    switch (Cmd) {
    case 2:
        // ����������� ���������� �� ������
        if (Prm)
            res = SCULdrv->SendSetAudioChan(1);
        break;
    case 3:
        // ����������� ���������� �� ��������
        if (Prm)
            res = SCULdrv->SendSetAudioChan(2);
        break;
    case 5:
        // ���/���� K1
            res = SCULdrv->SendKOnOff(1, 1 == Prm);
        break;
    case 6:
        // ���/���� K2
            res = SCULdrv->SendKOnOff(2, 1 == Prm);
        break;
    case 11:
        // �������� ����� ������������ 1
        if (Prm)
            res = SCULdrv->SendSecureRST();
        break;
    case 12:
        // �������� ����� ������������ 2
        if (Prm)
            res = SCULdrv->SendSecureRST();
        break;
    case 13:
        // ������ ���������
        if (Prm)
            res = SCULdrv->SendGetStatus();
        break;
    case 20:
        // Reset ���������
        if (Prm)
            res = SCULdrv->SendTerminalRST();
        break;
    default:
        res = false;
    }
    return res;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
DWORD SEAFlagsToRTUTS(DWORD SEAFlags)
{
    DWORD res;

    res = 0;
    if (SEAFlags & 1) res |= 1 << DI_1;      // ��������� ����������
    if (SEAFlags & (1 << 1)) res |= 1 << DI_ALARM1;      // ���� ������������ 1
    if (SEAFlags & (1 << 2)) res |= 1 << DI_ALARM2;      // ���� ������������ 1
    if (SEAFlags & (1 << 3)) res |= 1 << DI_2;           // ���� �������� ������
    if (SEAFlags & (1 << 4)) res |= 1 << DI_3;           // ���� �������� ������
    if (SEAFlags & (1 << 5)) res |= 1 << DI_4;           // ���� ������ ���������
    if (SEAFlags & (1 << 6)) res |= 1 << DI_5;           // ����� ������
    if (SEAFlags & (1 << 7)) res |= 1 << DI_6;           // ����� �����
    if (SEAFlags & (1 << 8)) res |= 1 << DI_7;           // ������� ������
    if (SEAFlags & (1 << 9)) res |= 1 << DI_8;           // ������ 15 �� (������ ���������)
    if (SEAFlags & (1 << 11)) res |= 1 << DI_INTR1;      // ������ �����
    if (SEAFlags & (1 << 12)) res |= 1 << DI_INTR2;      // ������ ��������� ���������
    if (SEAFlags & (1 << 13)) res |= 1 << DI_KEY2;       // ����� �� ������ �����
    if (SEAFlags & (1 << 14)) res |= 1 << DI_KEY3;       // ����� ��
    if (SEAFlags & (1 << 15)) res |= 1 << DI_15;         // ���� �1 (�������)
    if (SEAFlags & (1 << 16)) res |= 1 << DI_16;         // ���� �2 (���������)
    if (SEAFlags & (1 << 17)) res |= 1 << DI_PHASEA;       // �������� ����

    return res;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool v2mSetLiftState(WORD RTUaddr, DWORD RTUTS, DWORD state, DWORD &RTUTrF)
{
    int hdev;
    int BlocksNum;
    int hblock;
    int BType;
    int TagsNum;
    int TSTag;
    BYTE BitVal;
    BYTE oldBitVal;
    time_sms tsms;
    bool v2mOK = false;

#ifdef WIN32
    struct _timeb tsmsb;    // ����� �������� ������
#else
    struct timeb tsmsb;
#endif

    if (0 > (hdev = GetDevHandleBySno(RTUaddr))) return false;

    SetDevLinkStatus(hdev,1);                              // ������ v2mSetLinkStatus(RTUaddr);
    IncDevStat(hdev, 0);

    RTUTrF = 0;

    TSTag = 0;
    BlocksNum = GetDevBlocksNum(hdev);                                              // �������� ���-�� ������
    if (BlocksNum <= 0) return false;
    for (hblock = 0; hblock < BlocksNum; hblock++) {
        BType = GetBtype(hdev, hblock);                                             // �������� ��� �����
        if (TS == BType) {                                                          // ���� ��� ���� TS
            TagsNum = GetBlockTagsNum(hdev, hblock);                                // �������� ���-�� ����� � �����
            if (0 == GetBpactime(hdev, hblock)) v2mOK = false;                      // �����������, ��� ������ � v2m ����������, � �� ���������� ����� ������
            else v2mOK = true;

#ifdef WIN32
            _ftime(&tsmsb);
#else
            ftime(&CreatTime);
#endif
            tsms.sec = tsmsb.time;
            tsms.msec = tsmsb.millitm;
            if (TSTag < 64) {
                for (int tag = 0; tag < TagsNum; tag++) {

                    if ((TSTag >= 0) && (TSTag <= 31)) {                            // ������������ ����� ��������� ��������
                        if (RTUTS & (1 << TSTag)) BitVal = 1;                    // ��������� ����� ��������� ����
                        else BitVal = 0;

                        if (v2mOK) {
                            oldBitVal = GetTagD(hdev, hblock, tag);                        // �������� ������� ��������� ����
                            if (oldBitVal != BitVal)
                                RTUTrF |= (1 << TSTag);
                        }

                    }

                    if ((TSTag >= 32) && (TSTag <= 63)) {               // ������������ ����� ��������� �����������
                        if (TSTag - 32 == state) BitVal = 1;
                        else BitVal = 0;
                    }

                    SetTagD(hdev, hblock, tag, BitVal);                 // ���������� ��������� ����
                    TSTag++;
                }
                SetBtime(hdev, hblock, &tsms);
                SetBpactime(hdev, hblock, tsms.sec);
            }
            else TSTag += TagsNum;
        }
    }

    // ������� ���������� ��������� �� �������� ������
    char str[250];
    snprintf(str, sizeof(str), "v2m write to RTU[%d] TS=%.4X state=%X", RTUaddr, RTUTS, state);
    Logger->LogMessage(str, LOG_DBG2);

    return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool v2mCheckCmd(WORD RTUaddr, TPrtSCUL *SCULdrv)
{
    int hdev;
    int BlocksNum;
    int hblock;
    int BType;
    int TUTag;
    int TagVal = 0;
    int TagsNum;

    if (0 > (hdev = GetDevHandleBySno(RTUaddr))) return false;
    if (!IsDevOnControl(hdev))   return true;

    TUTag = 0;

    BlocksNum = GetDevBlocksNum(hdev);                                              // �������� ���-�� ������
    if (BlocksNum <= 0) return false;
    for (hblock = 0; hblock < BlocksNum; hblock++) {
        BType = GetBtype(hdev, hblock);                                             // �������� ��� �����
        if (TU == BType) {
            TagsNum = GetBlockTagsNum(hdev, hblock);                                // �������� ���-�� ����� � �����
            if ((0 <= GetBlockChangeCount(hdev, hblock)) && (TUTag<32)) {           // ���� ���� ��������� � �����
                for (int tag = 0; tag < TagsNum; tag++) {                           // ������������� ��� ���� � �����
                    if (IsTagDChanged(hdev, hblock, tag)) {                         // ���� ��������� ���� ����������
                        TagVal = GetTagD(hdev, hblock, tag);                        // �������� �������� ��������� ����
                        RTUCommandProcessing(TUTag, TagVal, SCULdrv);               // ������������ �������� ������� (��������������� ������������� ����� - ����)
                        IncDevStat(hdev, 3);
                        ResetTagDChangeFlag(hdev, hblock, TUTag);                   // ���������� ������� ��������� � ����
                    }
                    TUTag++;
                }
            }
            else TUTag += TagsNum;
        }
    }

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool v2mSetLinkStatus(WORD RTUaddr)
{
    int hdev;
    if (0 > (hdev = GetDevHandleBySno(RTUaddr))) return false;
    SetDevLinkStatus(hdev, 1);
    IncDevStat(hdev, 0);
    return true;
}

//-----------------------------------------------------------------------------
//  ������������� ��� ���������� ���������, ������� � fromRTU
//  ���� �����, � ������� �� ���� ����� ������ threshold ������
//  � ��� ��� �� ������������� (������ � ����)
//  ���������� ����� ������ ��, ��� -1, ���� ����� ���
//-----------------------------------------------------------------------------
int GetRTUWithConnectTimeOut(int fromRTU, int threshold, int &OutTime)
{
    int res = -1;
    int hdev;
    int DQ;
    int rtuto;
    int rtutimer;

    DQ = GetDevsQuantity();                 // ������ ������� ����� ���������
    hdev = fromRTU;
    if (hdev >= DQ) hdev = 0;
    for (int i = 0; i < DQ; i++) {          // ���������� ��� ���������� ���������
        hdev++;
        if (hdev >= DQ) hdev = 0;
        rtuto = GetDevElapsedTime(hdev);
        rtutimer = GetDevStat(hdev,11);
        if ((rtuto > threshold) && (!rtutimer)) {
            res = hdev;
            OutTime = rtuto;
            break;
        }
    }
    return res;
}

//-----------------------------------------------------------------------------
//  ������� ������ �� �� RTUNum �� TimerVal ������
//-----------------------------------------------------------------------------
void SetRTUTimer(int RTUNum, int TimerVal)
{
    SetDevStat(RTUNum, 11, TimerVal);
}

//-----------------------------------------------------------------------------
//  ���������� ��������� ����� �� � ������� RTUNum
//-----------------------------------------------------------------------------
int GetRTUDevNo(int RTUNum)
{
    return GetDevSno(RTUNum);
}