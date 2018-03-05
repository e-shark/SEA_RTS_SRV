#ifndef ATCommanderH
#define ATCommanderH
//*****************************************************************************
//  Модуль управления модемом посредством AT команд
//      DAV     I.1028г.
//*****************************************************************************

#include "Logger.h"

//-----------------------------------------------------------------------------
class TATCommander
{
private:
    void* hcom;                                     // указатель на открываемый COM порт
    int TimeOutCom;                                 // timeout по порту
    int ComNum;
    int ComBaud;
    char PortMode[100];

    bool PortOpen(void);
    void PortClose(void);
    void PortFlush(void);

protected:
    TLogger *pLogger;                               // Объект, отвечающий за логирование

    bool LogMessage(std::string Message, int Mask);

    int SendStr(char *str);
    int GetAnswer(char *buf, int buflen, int TOutSec);

public:
    ~TATCommander();
    TATCommander();
    void SetLogger(TLogger *pLgg);

    void SetComNum(int Num);
    void SetComBaud(int Baud);
    void SetComMode(char* ModeStr);

    bool TryCall(char* PhoneNumber, int TimeOutSec);

    bool SEATryRemoteReset(char* PhoneNumber, int ConnectTimeOutSec);
    void FinishCall();

};

#endif