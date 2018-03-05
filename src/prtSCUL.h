/*------------------------------------------------------------------------------
*	Май 2017г.
*	Диденко А.В.
*	Модуль драйвера протокола Sea SCUL.
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
    int InputCount;                                 // Кол принятых символом в текущем принимаемом пакете
    int InputState;                                 // Состояние входного буфера: 0-ожидание начала пакета; 1-ожидание конца пакета
    bool StrangeProtocol;
    pthread_mutex_t hSendMutex;
    time_t LastSend;                                // Когда была последняя посылка на табло

protected:
    char InBuf[1024];                                       // Буфер приемника
    TLogger *pLogger;                                       // Объект, отвечающий за логирование
    std::string SysDevType;

    void ClearInBuf(void);                                  // Очистить входной буфер
    void ProcessPacket(char *Data, int Len);                // Обработать принятый от терминала (лифтового контроллера) пакет пакет
    void ProcessStrangePacket(char *Data, int Len);         // Обработать принятый от терминала (лифтового контроллера) пакет пакет
    bool SendCMD(char *CMD, char *Param = NULL);            // Отослать команду

public:
    TPrtSCUL(void);
    ~TPrtSCUL();

    bool AutoCorrectionTime;                                    // Признак необходимости производить автокоррекцию времени терминала
    bool TUGrand;                                               // Разрешение команд телеуправления

    std::string SysSwVer;
    std::string SysDeviceID;
    int KeepaliveTime;                                          // Интервал (в секундах), через который должны уходить посылки на терминал для поддержания связи

    TCommandPoll *CmdRxPoll;                                    // Очередь комманд (сюда складываем принимаемые пакеты от лифта)
    TCommandPoll *CmdTxPoll;                                    // Очередь комманд (сюда складываем пакеты, которые надо отправить лифту)

    void SetLogger(TLogger *pLgg);                              // Установить логгер
    void ProcessChar(char c);                                   // Обработать очередной входной символ
    bool IsKeepaliveTimeOut(void);
    void NoteSendTime(void);

    bool SendGetStatus(void);                                   // Запрос статуса состояния
    bool SendStatusACK(void);                                   // Подтверждение получения статуса состояния
    bool SendKOnOff(int Kn, bool On);                           // Включить/Выключить реле
    bool SendSetAudioChan(int Chan);                            // Переключение аудиоканала : 1- кабина, 2- машинное
    bool SendSecureRST(void);                                   // Сбросить триггеры сигнализации
    bool SendSetTime(int Year, int Month, int Day, int Hour, int Min, int Sec); // Установить часы
    bool SendSetCurrentTime(void);                              // Синхронизировать время со временем серевера диспетчера
    bool SendTerminalRST(void);                                 // Перезагрузить терминал

};

//----------------------------------------------------------------------------
time_t TimeFromSCULPacket(sSCULPacket*Packet);

#endif
