/*------------------------------------------------------------------------------
*	��� 2017�.
*	������� �.�.
*	������ �������� ��������� Sea SCUL.
*----------------------------------------------------------------------------*/

#ifndef prtSCULH
#define prtSCULH

#include <time.h>

#include "Logger.h"
#include "oswr.h"
#include "CmdPoll.h"

//----------------------------------------------------------------------------
typedef enum {scUnknown, scRS, scSN, scSS, scAS, scRO, scSO, scWO, scON, scOFF, scSA, scSC, scRT, scMR, scMA, scMW, scAN, scTR, scERR, scStrng }eSCULcmd;

class sSCULPacket {
protected:
    eSCULcmd fCmdType;
public:
    sSCULPacket(void)  { fCmdType = scUnknown; };
    std::string DevType;
    std::string devID;
    unsigned char prgVer;
    unsigned char tYear;
    unsigned char tMonth;
    unsigned char tDay;
    unsigned char tHour;
    unsigned char tMin;
    unsigned char tSec;
    eSCULcmd CmdType(void)  { return fCmdType; };
    virtual ~sSCULPacket();
};

class sSCULPacket_SN : public sSCULPacket {
public:
    sSCULPacket_SN(void)  { 
        fCmdType = scSN; Mode = 0; Flags = 0; };
    int Flags;
    int Mode;
    virtual ~sSCULPacket_SN();
};

class sSCULPacket_AN : public sSCULPacket {
public:
    sSCULPacket_AN(void)  {fCmdType = scSN; cmd = scUnknown; err = 0; };
    eSCULcmd cmd;
    int err;
    virtual ~sSCULPacket_AN();
};

class sSCULPacket_STRNG : public sSCULPacket {
public:
    sSCULPacket_STRNG(void) { fCmdType = scStrng;};
    virtual ~sSCULPacket_STRNG() {};
};

//----------------------------------------------------------------------------
class TPrtSCUL
{
private:
    int InputCount;                                 // ��� �������� �������� � ������� ����������� ������
    int InputState;                                 // ��������� �������� ������: 0-�������� ������ ������; 1-�������� ����� ������
    bool StrangeProtocol;
    pthread_mutex_t hSendMutex;
    time_t LastSend;                                // ����� ���� ��������� ������� �� �����

protected:
    char InBuf[1024];                                       // ����� ���������
    TLogger *pLogger;                                       // ������, ���������� �� �����������
    std::string SysDevType;

    void ClearInBuf(void);                                  // �������� ������� �����
    void ProcessPacket(char *Data, int Len);                // ���������� �������� �� ��������� (��������� �����������) ����� �����
    void ProcessStrangePacket(char *Data, int Len);         // ���������� �������� �� ��������� (��������� �����������) ����� �����
    bool SendCMD(char *CMD, char *Param = NULL);            // �������� �������

public:
    TPrtSCUL(void);
    ~TPrtSCUL();

    bool AutoCorrectionTime;                                    // ������� ������������� ����������� ������������� ������� ���������
    bool TUGrand;                                               // ���������� ������ ��������������

    std::string SysSwVer;
    std::string SysDeviceID;
    int KeepaliveTime;                                          // �������� (� ��������), ����� ������� ������ ������� ������� �� �������� ��� ����������� �����

    TCommandPoll *CmdRxPoll;                                    // ������� ������� (���� ���������� ����������� ������ �� �����)
    TCommandPoll *CmdTxPoll;                                    // ������� ������� (���� ���������� ������, ������� ���� ��������� �����)

    void SetLogger(TLogger *pLgg);                              // ���������� ������
    void ProcessChar(char c);                                   // ���������� ��������� ������� ������
    bool IsKeepaliveTimeOut(void);
    void NoteSendTime(void);

    bool SendGetStatus(void);                                   // ������ ������� ���������
    bool SendStatusACK(void);                                   // ������������� ��������� ������� ���������
    bool SendKOnOff(int Kn, bool On);                           // ��������/��������� ����
    bool SendSetAudioChan(int Chan);                            // ������������ ����������� : 1- ������, 2- ��������
    bool SendSecureRST(void);                                   // �������� �������� ������������
    bool SendSetTime(int Year, int Month, int Day, int Hour, int Min, int Sec); // ���������� ����
    bool SendSetCurrentTime(void);                              // ���������������� ����� �� �������� �������� ����������
    bool SendTerminalRST(void);                                 // ������������� ��������

};

//----------------------------------------------------------------------------
time_t TimeFromSCULPacket(sSCULPacket*Packet);

#endif
