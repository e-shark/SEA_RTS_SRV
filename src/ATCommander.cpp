
//*****************************************************************************
//  Модуль управления модемом посредством AT команд
//      DAV     I.1028г.
//*****************************************************************************

#include <time.h>
#include "ATCommander.h"
#include "osuart.h"
#include "utils.h"

//-----------------------------------------------------------------------------
//  Конструктор класса
//-----------------------------------------------------------------------------
TATCommander::TATCommander()
{
    hcom = INVALID_HANDLE_VALUE;
    pLogger = NULL;
    ComNum = 1;
    ComBaud = 9600;
    TimeOutCom = 100;
    sprintf(PortMode, "baud=%d parity=N data=8 stop=1", ComBaud);
    //sprintf(PortMode, "baud=9600 parity=N data=8 stop=1");
}

//-----------------------------------------------------------------------------
//  Деструктор класса
//-----------------------------------------------------------------------------
TATCommander::~TATCommander()
{
    FinishCall();
    PortClose();
}

//----------------------------------------------------------------------------
//  Установить логгер
//-----------------------------------------------------------------------------
void TATCommander::SetLogger(TLogger *pLgg)
{
    if (pLgg) pLogger = pLgg;
}

//----------------------------------------------------------------------------
//  Вывод сообщение в лог 
//----------------------------------------------------------------------------
bool TATCommander::LogMessage(std::string Message, int Mask)
{
    bool res = true;
    std::string tempmsg;
    tempmsg = "COM";
    tempmsg += IntToStr(ComNum);
    tempmsg += ": ";
    tempmsg += Message;
    if (pLogger) res = pLogger->LogMessage(tempmsg, Mask);
    return res;
}

//----------------------------------------------------------------------------
//  Выбрать COM порт
//-----------------------------------------------------------------------------
void TATCommander::SetComNum(int Num)
{
    char s[50];
    //PortClose();
    if (Num != ComNum) {
        sprintf(s, "change COM from %d to %d", ComNum, Num);
        LogMessage(s, LOG_DBG3);
    }
    ComNum = Num;
}

//----------------------------------------------------------------------------
//  Выбрать скорость обмена по COM порту
//-----------------------------------------------------------------------------
void TATCommander::SetComBaud(int Baud)
{
    char s[50];
    //PortClose();
    if (Baud != ComBaud) {
        sprintf(s, "change COM from %d to %d", ComBaud, Baud);
        LogMessage(s, LOG_DBG3);
        ComBaud = Baud;
        sprintf(PortMode, "baud=%d parity=N data=8 stop=1", ComBaud);
    }
}

//----------------------------------------------------------------------------
//  Установить режим работы COM порта
//  В качестве параметра строки вида "baud=9600 parity=N data=8 stop=1"
//  (Формат строки описан в osUartOpen в модуле osuart.h)
//----------------------------------------------------------------------------
void TATCommander::SetComMode(char* ModeStr)
{
    if (!ModeStr) return;
    strncpy(PortMode, ModeStr, sizeof(PortMode));
}


//----------------------------------------------------------------------------
//  Открыть COM порт
//----------------------------------------------------------------------------
bool TATCommander::PortOpen(void)
{
    char scn[30];
    sprintf(scn,"\\\\.\\com%d", ComNum);
    hcom = osUartOpen(scn, PortMode);
    if (INVALID_HANDLE_VALUE == hcom) {
        LogMessage("port open error! ", LOG_ERR2);
        return false;
    }
    LogMessage("port open", LOG_DBG3);
    osSleep(100);
    return true;
}

//----------------------------------------------------------------------------
//  Закрыть COM порт
//----------------------------------------------------------------------------
void TATCommander::PortClose(void)
{
    if (hcom) {
        osUartClose(hcom);
        hcom = INVALID_HANDLE_VALUE;
    }
    LogMessage("port close", LOG_DBG3);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void TATCommander::PortFlush(void)
{
    if (hcom) osUartFlush(hcom, 3);   // flash all
}

//----------------------------------------------------------------------------
//  Отправить строку модему (в COM порт)
//----------------------------------------------------------------------------
int TATCommander::SendStr(char *str)
{
    int res = 0;
    int rw;
    char cs[100];
    std::string s;
    int slen;

    slen = strlen(str);
    memset(cs, 0, sizeof(cs));
    if (sizeof(cs) - 1 < slen)
        rw = sizeof(cs) - 1;
    else
        rw = slen;
    strncpy(cs, str, rw);
    s = "Commander: send '";
    s += cs;
    s += "'";
    LogMessage(s, LOG_DBG3);

    // отправляем пакет по TCP
    rw = osUartWrite(hcom, (unsigned char*)str, slen, TimeOutCom);

    // проверяем, получилось ли отправить пакет
    if (rw < 0) {
        LogMessage("Commander error: write fault! ", LOG_ERR3);
        res = -1;
    }
    return res;
}

//----------------------------------------------------------------------------
//  Читает ответ вида #CR#LF[ответ]#CR#LF
//  Возвращает длину ответа (без учета #CR#LF, только текст самого ответа)
//  или код ошибки (<0):
//      -1  - ошибка сокета
//      -2  - не дождались ACK (#CR#LF)
//----------------------------------------------------------------------------
#define MaxReadNum 100
int TATCommander::GetAnswer(char *buf, int buflen, int TOutSec)
{
    int res = 0;
    int rr;
    char c1, c2;
    char readbuf[MaxReadNum + 10];
    int num = 0;
    bool bACK;
    time_t starttime, now;
    std::string s;

    memset(buf, 0, buflen);

    time(&starttime);

    // читаем первый ACK 'OK'

    num = 0;
    bACK = false;
    do {
        rr = osUartRead(hcom, (unsigned char*)&c2, 1, TimeOutCom);                    // Пытаемся прочитать символ

        if (rr < 0) goto readerr;
        if (rr > 0) {
            num++;
            if ((num > 1) && ('\r' == c1) && ('\n' == c2))
                bACK = true;
            else
                c1 = c2;
        }
        time(&now);
    } while ((!bACK) && ((now - starttime) < TOutSec));

    if (!bACK) return -2;    //  не дождались ACK

                             // читаем ответ до второго ACK 'OK'

    memset(readbuf, 0, sizeof(readbuf));
    num = 0;
    bACK = false;
    do {
        rr = osUartRead(hcom, (unsigned char*)&c2, 1, TimeOutCom);                    // Пытаемся прочитать символ
        if (rr < 0) goto readerr;
        if (rr > 0) {
            readbuf[num] = c2;
            num++;
            if ((num > 1) && ('\r' == c1) && ('\n' == c2))
                bACK = true;
            else
                c1 = c2;
        }
        time(&now);
    } while ((!bACK) && (num < MaxReadNum) && ((now - starttime) < TOutSec));

    if (!bACK) return -2;       // не дождались ACK

    rr = num - 2;               // учитываем завершающее ACK
    if (rr) {
        if (buflen - 1 < rr) rr = buflen - 1;
        memcpy(buf, readbuf, rr);
    }
    buf[rr] = 0;

    s = "Commander: read '";
    s += buf;
    s += "'";
    LogMessage(s, LOG_DBG3);

    return rr;

readerr:
    LogMessage("Commander error: read fault!", LOG_ERR3);
    return -1;
}


//----------------------------------------------------------------------------
//  Предпринимается попытка совершить voice звонок
//----------------------------------------------------------------------------
bool TATCommander::TryCall(char* PhoneNumber, int TimeOutSec)
{
    bool res = false;
    char Buf[100];
    char s[200];
    snprintf(Buf, sizeof(Buf), "ATD%s;\r\n", PhoneNumber);
    SendStr(Buf);
    GetAnswer(Buf, sizeof(Buf), TimeOutSec);
    if (strstr(strupr(Buf), "OK")) {
        res = true;
        snprintf(s, sizeof(s), "call to %s successful", PhoneNumber);
    }
    else {
        snprintf(s, sizeof(s), "call to %s fail with %s", PhoneNumber, Buf);
    }
    LogMessage(s, LOG_DBG3);
    return res;
}

//----------------------------------------------------------------------------
//  Завершить voice звонок
//----------------------------------------------------------------------------
void TATCommander::FinishCall()
{
    LogMessage("finish call", LOG_DBG3);
    SendStr("ATH\r\n");
}

//----------------------------------------------------------------------------
//  Осуществить попытку произвести удаленный сброс устройства SEA SCUL
//  (Выполняет голосовой звонок на устройство и выдает DTMF команду сброса "A7")
//----------------------------------------------------------------------------
bool TATCommander::SEATryRemoteReset(char* PhoneNumber, int ConnectTimeOutSec)
{
    bool res = 0;
    char Buf[100];
    char s[200];

    if (PortOpen()) {
        SendStr("ATQ0E0\r\n");                                              // Конфигурируем модем (включить подтверждения и выключить эхо)
        osSleep(500);
        PortFlush();
        if (TryCall(PhoneNumber, ConnectTimeOutSec)) {                      // Осуществляем голосовой вызов на устройство
            SendStr("at + VTS = a\r\nat + VTS = 7\r\n");                    // Выдаем DTMF команду на устройство
            GetAnswer(Buf, sizeof(Buf), 10);
            if (strstr(strupr(Buf), "OK")) {
                snprintf(s, sizeof(s), "send DTMF successful");
                res = true;
                osSleep(5000);
            }
            else {
                snprintf(s, sizeof(s), "send DTMF fail with %s", Buf);
            }
            LogMessage(s, LOG_DBG3);
        }
        FinishCall();                                                       // Завершаем голосоаой вызов
    }
    PortClose();
    return res;
}


