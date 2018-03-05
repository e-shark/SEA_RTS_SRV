/*------------------------------------------------------------------------------
*	Май 2017г.
*	Диденко А.В.
*	Модуль драйвера протокола Sea SCUL.
*----------------------------------------------------------------------------*/

#include <string.h>

#include "prtSCUL.h"

//----------------------------------------------------------------------------
//  Переводит строку с бинарным отображением числа в число int
//----------------------------------------------------------------------------
int BinToInt(char* sf)
{
    int res = 0;
    char *pC = sf;
    while (*pC != 0) {
        res <<= 1;
        switch (*pC) {
        case '0': break;
        case '1': res |= 1; break;
        default: return res;
        }
        pC++;
    }
    return res;
}

//----------------------------------------------------------------------------
// Переводит строку с бинарным отображением числа
// c little - endian последовательностью бит
// в число int
//----------------------------------------------------------------------------
int BinToIntLE(char* sf)
{
    int res = 0;
    char *pC = sf;
    int n = 1;
    while (*pC != 0) {
        switch (*pC) {
        case '0': break;
        case '1': res += n; break;
        default: return res;
        }
        n<<=1;
        pC++;
    }
    return res;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
eSCULcmd ScanCMD(char *pCMDstr)
{
    if (!pCMDstr) return scUnknown;
    if (('R' == *(pCMDstr)) && ('S' == *(pCMDstr + 1))) return scRS;
    if (('S' == *(pCMDstr)) && ('N' == *(pCMDstr + 1))) return scSN;
    if (('S' == *(pCMDstr)) && ('S' == *(pCMDstr + 1))) return scSS;
    if (('A' == *(pCMDstr)) && ('S' == *(pCMDstr + 1))) return scAS;
    if (('R' == *(pCMDstr)) && ('O' == *(pCMDstr + 1))) return scRO;
    if (('S' == *(pCMDstr)) && ('O' == *(pCMDstr + 1))) return scSO;
    if (('W' == *(pCMDstr)) && ('O' == *(pCMDstr + 1))) return scWO;
    if (('O' == *(pCMDstr)) && ('N' == *(pCMDstr + 1))) return scON;
    if (('O' == *(pCMDstr)) && ('F' == *(pCMDstr + 1)) && ('F' == *(pCMDstr + 2))) return scOFF;
    if (('S' == *(pCMDstr)) && ('A' == *(pCMDstr + 1))) return scSA;
    if (('S' == *(pCMDstr)) && ('C' == *(pCMDstr + 1))) return scSC;
    if (('R' == *(pCMDstr)) && ('T' == *(pCMDstr + 1))) return scRT;
    if (('M' == *(pCMDstr)) && ('R' == *(pCMDstr + 1))) return scMR;
    if (('M' == *(pCMDstr)) && ('A' == *(pCMDstr + 1))) return scMA;
    if (('M' == *(pCMDstr)) && ('W' == *(pCMDstr + 1))) return scMW;
    if (('A' == *(pCMDstr)) && ('N' == *(pCMDstr + 1))) return scAN;
    if (('T' == *(pCMDstr)) && ('R' == *(pCMDstr + 1))) return scTR;
    if (('E' == *(pCMDstr)) && ('R' == *(pCMDstr + 1)) && ('R' == *(pCMDstr + 2))) return scERR;
    return scUnknown;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
time_t TimeFromSCULPacket(sSCULPacket*Packet)
{
    time_t res = time(NULL);
    tm tm;
    if (Packet) {
#ifdef WIN32
        localtime_s(&tm, &res);
#else
        localtime_r(&tm, &res);
#endif
        tm.tm_year = Packet->tYear + 100;
        tm.tm_mon = Packet->tMonth - 1;
        tm.tm_mday = Packet->tDay;
        tm.tm_hour = Packet->tHour;
        tm.tm_min = Packet->tMin;
        tm.tm_sec = Packet->tSec;
        res = mktime(&tm);
    }
    return res;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
sSCULPacket::~sSCULPacket()
{
    fCmdType = scUnknown;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
sSCULPacket_SN::~sSCULPacket_SN()
{
    Mode = 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
sSCULPacket_AN::~sSCULPacket_AN()
{
    err = 0;
}

//********************************************************************************************************

//----------------------------------------------------------------------------
//  Конструктор класса
//----------------------------------------------------------------------------
TPrtSCUL::TPrtSCUL(void)
{
    pLogger = NULL;
    hSendMutex = NULL;
    osInitThreadMutex(&hSendMutex);
    SysDevType = "SKUL";
    SysSwVer = "111";
    SysDeviceID = "111111111111111";
    ClearInBuf();
    AutoCorrectionTime = true;
    TUGrand = true;
    KeepaliveTime = 240;
    time(&LastSend);
    CmdRxPoll = new TCommandPoll();
    CmdTxPoll = new TCommandPoll();
}

//----------------------------------------------------------------------------
//  Деструктор класса
//----------------------------------------------------------------------------
TPrtSCUL::~TPrtSCUL()
{
    delete CmdRxPoll;
    delete CmdTxPoll;
    osDestroyThreadMutex(&hSendMutex);
}

//----------------------------------------------------------------------------
//  Очистка входного буфера
//----------------------------------------------------------------------------
void TPrtSCUL::ClearInBuf(void)
{
    memset(InBuf, 0, sizeof(InBuf));
    InputCount = 0;
    InputState = 0;
    StrangeProtocol = false;
}

//----------------------------------------------------------------------------
//  Установить логгер
//-----------------------------------------------------------------------------
void TPrtSCUL::SetLogger(TLogger *pLgg)
{
    if (pLgg) pLogger = pLgg;
}

//----------------------------------------------------------------------------
//  Обработка очередного входящего символа
//----------------------------------------------------------------------------
void TPrtSCUL::ProcessChar(char c)
{
    char *pC;

    switch (InputState) {
    case 0:
        if (InputCount == 11) {
            memmove(InBuf, InBuf + 1, 11);
            InputCount--;
        }
        InBuf[InputCount] = c;
        InputCount++;
        if (strcmp(InBuf, "/STARTDATA/") == 0) {
            InputState++;
        }
        if (strstr(InBuf, "CONNECT/") ) {
            StrangeProtocol = true;
            InputState++;
        }
        if (strstr(InBuf, "PING/") ) {
            StrangeProtocol = true;
            InputState++;
        }
        break;
    case 1:
        InBuf[InputCount] = c;
        InputCount++;
        if (InputCount >= 11 + 9) {
            if (InputCount < sizeof(InBuf) - 1){
                pC = InBuf + InputCount - 9;            // "/ENDDATA/"
                if (strcmp(pC, "/ENDDATA/") == 0) {
                    ProcessPacket(InBuf, InputCount);
                    ClearInBuf();
                }
                //if (StrangeProtocol)
                    if (strcmp(pC, "/NO DATA/") == 0) {
                        StrangeProtocol = true;
                        ProcessStrangePacket(InBuf, InputCount);
                        ClearInBuf();
                    }
            }
            else {
                ClearInBuf();
            }
        }
        break;
    }
}

//----------------------------------------------------------------------------
//  Проверяет, нужно ли корректировать время на терминале
//----------------------------------------------------------------------------
bool CheckTime(char *ts, int limit)
{
    time_t ct,pt;
    tm ctm;
    tm ptm;
    int n;
    int tYear, tMonth, tDay, tHour, tMin, tSec;

    if (!ts) return true;

    n = sscanf(ts,"%02d%02d%02d%02d%02d%02d", &tYear, &tMonth, &tDay, &tHour, &tMin, &tSec);
    if (n<6) return true;
    time(&ct);
#ifdef WIN32
    localtime_s(&ctm, &ct);
#else
    localtime_r(&ct, &ctm);
#endif
    ptm = ctm;
    ptm.tm_year = tYear+100;
    ptm.tm_mon = tMonth-1;
    ptm.tm_mday = tDay;
    ptm.tm_hour = tHour;
    ptm.tm_min = tMin;
    ptm.tm_sec = tSec;
    pt = mktime(&ptm);
    if (abs(pt - ct) > limit)
        return true;
    else
        return false;
}

//----------------------------------------------------------------------------
//  Обработка приянтого пакета паета
//----------------------------------------------------------------------------
void TPrtSCUL::ProcessPacket(char *Data, int Len)
{
    int err = 0;
    char *pC1, *pCE, *pC2, *pC3;
    char *pDevType;
    char *pSwVer;
    char *pDeviceID;
    char *pTimeStamp;
    char *pCMD;
    sSCULPacket *Packet = NULL;
    int tYear, tMonth, tDay, tHour, tMin, tSec;
    eSCULcmd CmdType;
    char ls[1000];
    int ln = 0;

    if (pLogger) {
        memset(ls, 0, sizeof(ls));
        ln = sprintf(ls, "SCUL recive pack : ");
        strncpy(ls + ln, (char*)Data, Len);
        pLogger->LogMessage(ls, LOG_DBG2);
    }

    pC1 = strstr(Data, "/STARTDATA/");      // Проверяем наличие начала пакета
    if (!pC1) { err = 1; goto ProcessPacketErr; };
    pC1 += 11;
    pCE = strstr(pC1, "/ENDDATA/");         // Проверяем наличие конца пакета
    if (!pCE) { err = 2; goto ProcessPacketErr; };

    // Ищем ТИП_ПРИБОРА
    pDevType = pC1;
    pC1 = strchr(pDevType, '/');
    if (!pC1 || (pC1 >= pCE)) { err = 3; goto ProcessPacketErr; };
    *pC1 = 0;
    pC1++;

    // Ищем ВЕРСИЮ_ПО
    pSwVer = pC1;
    pC1 = strchr(pC1, '/');
    if (!pC1 || (pC1 >= pCE)) { err = 4; goto ProcessPacketErr; };
    *pC1 = 0;
    pC1++;

    // Ищем ИДЕНТИФИКАТОР ПРИБОРА
    pDeviceID = pC1;
    pC1 = strchr(pC1, '/');
    if (!pC1 || (pC1 >= pCE)) { err = 5; goto ProcessPacketErr; };
    *pC1 = 0;
    pC1++;

    // Ищем ВРЕМЯ
    pTimeStamp = pC1;
    pC1 = strchr(pC1, '/');
    if (!pC1 || (pC1 >= pCE)) { err = 6; goto ProcessPacketErr; };
    *pC1 = 0;
    pC1++;

    // Ищем ТИП_ДАННЫХ
    pCMD = pC1;
    pC1 = strchr(pC1, '/');
    if (!pC1 ) { err = 7; goto ProcessPacketErr; };
    *pC1 = 0;
    pC1++;

    // Ищем начало сообщения
    if (pC1 - 1 < pCE) {
        // Тут PC1 указывает на начало сообщения
        CmdType = ScanCMD(pCMD);
        if (scAN == CmdType) {
            // пакет квитирования команды

            // Ищем конец команды, на которую пришло подстверждение
            pC2 = strchr(pC1, '/');
            if (!pC2) { err = 10; goto ProcessPacketErr; };
            *pC2 = 0;
            pC2++;
            /*
            // Ищем конец кода ошибки
            pC3 = strchr(pC2, '/');
            if (!pC3 || (pC3 >= pCE)) { err = 11; goto ProcessPacketErr; };
            *pC3 = 0;
            pC1++;
            */

            Packet = (sSCULPacket*) new sSCULPacket_AN();
            ((sSCULPacket_AN*)Packet)->cmd = ScanCMD(pC1);

        }

        if (scSN == CmdType) {
            // пакет слова состояния

            // Ищем конец флагов
            pC2 = strchr(pC1, '/');
            if (!pC2 || (pC2 >= pCE)) { err = 12; goto ProcessPacketErr; };
            *pC2 = 0;
            pC2++;

            // Ищем конец кода ошибки
            pC3 = strchr(pC2, '/');
            if (!pC3) { err = 13; goto ProcessPacketErr; };
            *pC3 = 0;
            pC3++;

            Packet = (sSCULPacket*) new sSCULPacket_SN();
            ((sSCULPacket_SN*)Packet)->Flags = BinToIntLE(pC1);
            if (sscanf(pC2,"%02X",&((sSCULPacket_SN*)Packet)->Mode) <1) { err = 14; goto ProcessPacketErr; };
        }

        if (Packet) {
            ((sSCULPacket*)Packet)->DevType = pDevType;
            ((sSCULPacket*)Packet)->devID = pDeviceID;
            ((sSCULPacket*)Packet)->prgVer = atoi(pSwVer);
            int iYear, iMonth, iDay;
            int iHour, iMin, iSec;
            if ( 6 == sscanf(pTimeStamp, "%02d%02d%02d%02d%02d%02d", &iYear, &iMonth, &iDay, &iHour, &iMin, &iSec) ) {
                ((sSCULPacket*)Packet)->tYear = iYear;
                ((sSCULPacket*)Packet)->tMonth = iMonth;
                ((sSCULPacket*)Packet)->tDay = iDay;
                ((sSCULPacket*)Packet)->tHour = iHour;
                ((sSCULPacket*)Packet)->tMin = iMin;
                ((sSCULPacket*)Packet)->tSec = iSec;
            }else{ err = 11; goto ProcessPacketErr; }
        }

        if (scSN == CmdType) {
            {//++++++++++++++++++++++++++++++++++++++++++++++++
                char str[60];
                //((sSCULPacket_SN*)Packet)->Flags = 0x0ab;
                sprintf(str, "flags: %X", ((sSCULPacket_SN*)Packet)->Flags);
                if (pLogger) pLogger->LogMessage(str, LOG_DBG3);
            }//++++++++++++++++++++++++++++++++++++++++++++++++
            if (Packet) {
                CmdRxPoll->PushPacket(Packet);
            }
            SendStatusACK();
        }

        if (AutoCorrectionTime)
            if (CheckTime(pTimeStamp, 60 * 10)) SendSetCurrentTime();

    }

    return;

ProcessPacketErr:
    if (Packet) delete (sSCULPacket*)Packet;
    char errstr[60];
    sprintf(errstr, "Packet parse error: %d", err);
    if (pLogger) pLogger->LogMessage(errstr, LOG_ERR2);
    return;
}

//----------------------------------------------------------------------------
//  Обработка приянтого пакета чужого протокола
//----------------------------------------------------------------------------
void TPrtSCUL::ProcessStrangePacket(char *Data, int Len)
{
    char ls[1000];
    int ln = 0;
    int err;
    char *pCS, *pCE, *pC1, *pC2, *pC3, *pC4, *pC5;
    sSCULPacket *Packet = NULL;

    if (pLogger) {
        memset(ls, 0, sizeof(ls));
        ln = sprintf(ls, "SCUL recive strange pack : ");
        strncpy(ls + ln, (char*)Data, Len);
        pLogger->LogMessage(ls, LOG_DBG2);
    }
    pCS = strstr(Data, "CONNECT/");      // Проверяем наличие начала пакета
    if (pCS) {
        pC1 = pCS + 7;
    }
    else {
        pCS = strstr(Data, "PING/");      // Проверяем наличие начала пакета
        if (!pCS) { err = 1; goto ProcessStrangePacketErr; };
        pC1 = pCS + 4;
    }
    *pC1 = 0;
    pC1++;


    pC2 = strchr(pC1, '/');     
    if (!pC2) { err = 2; goto ProcessStrangePacketErr; };
    *pC2 = 0;
    pC2++;

    pC3 = strchr(pC2, '/');     
    if (!pC3) { err = 3; goto ProcessStrangePacketErr; };
    *pC3 = 0;
    pC3++;

    pC4 = strchr(pC3, '/');
    if (!pC4) { err = 4; goto ProcessStrangePacketErr; };
    *pC4 = 0;
    pC4++;

    pC5 = strchr(pC4, '/');
    if (!pC5) { err = 5; goto ProcessStrangePacketErr; };
    *pC5 = 0;
    pC5++;

    pCE = strstr(pC5, "/NO DATA/");         // Проверяем наличие конца пакета
    if (!pCE) { err = 6; goto ProcessStrangePacketErr; };
    *pCE = 0;
    pCE++;

    Packet = (sSCULPacket*) new sSCULPacket_STRNG();
    ((sSCULPacket*)Packet)->devID = pC1;
    ((sSCULPacket*)Packet)->prgVer = atoi(pC4);

    if (Packet) {
        CmdRxPoll->PushPacket(Packet);
    }

ProcessStrangePacketErr:
    return;
}

//----------------------------------------------------------------------------
//  Запихать в очередь команду на девайс
//----------------------------------------------------------------------------
bool TPrtSCUL::SendCMD(char *CMD, char *Param)
{
    char errstr[60];
    char packet[1024];
    std::string CMDstr;
    time_t t;
    tm*tm;
    int n1, n2;
    char ls[1000];
    int ln;

    if (!CMD) return true;

    memset(packet, 0, sizeof(CMDstr));
    CMDstr = CMD;
    if (Param) {
        CMDstr += "/";
        CMDstr += Param;
    }
    time(&t);
    tm = localtime(&t);
    n1 = snprintf(packet, sizeof(packet), "/STARTDATA/%s/%s/%s/%02d%02d%02d%02d%02d%02d/%s/ENDDATA/", SysDevType.c_str(), SysSwVer.c_str(), SysDeviceID.c_str(),
        tm->tm_year % 100, tm->tm_mon + 1, tm->tm_mday,
        tm->tm_hour, tm->tm_min, tm->tm_sec,
        CMDstr.c_str());

    CmdTxPoll->PushPacket(packet, n1);

    if (pLogger) {
        memset(ls, 0, sizeof(ls));
        ln = sprintf(ls, "SCUL create CMD : ");
        strncpy(ls + ln, (char*)packet, n1);
        pLogger->LogMessage(ls, LOG_DBG2);
    }
    return true;
}

//----------------------------------------------------------------------------
//  Отметить время отправки комадны лифту
//----------------------------------------------------------------------------
void TPrtSCUL::NoteSendTime(void)
{
    time_t t;
    time(&t);
    LastSend = t;
}

//----------------------------------------------------------------------------
//  Проверить, не пора ли сделать посылку на терминал
//----------------------------------------------------------------------------
bool TPrtSCUL::IsKeepaliveTimeOut(void)
{
    time_t t;
    int diff;

    if (0 == KeepaliveTime) return false;               // не нужна поддержка соединения

    time(&t);
    diff = t - LastSend;
    if (diff > KeepaliveTime) 
        return true;
    else 
        return false;
}

//----------------------------------------------------------------------------
//  Послать команду "Получить статус"
//----------------------------------------------------------------------------
bool TPrtSCUL::SendGetStatus(void)
{
    if (pLogger) pLogger->LogMessage("SCUL Send CMD: Get Status (RS)", LOG_WRK3);
    return SendCMD("RS");
}

//----------------------------------------------------------------------------
//  Послать "Подтверждение получения статуса "
//----------------------------------------------------------------------------
bool TPrtSCUL::SendStatusACK(void)
{
    if (pLogger) pLogger->LogMessage("SCUL Send CMD: Status ACK (AS)", LOG_WRK3);
    return SendCMD("AS");
}

//----------------------------------------------------------------------------
//  Послать команду "Получить статус"
//----------------------------------------------------------------------------
bool TPrtSCUL::SendKOnOff(int Kn, bool cmdOn)
{
    char errstr[50];
    bool res = false;
    char prm[3];
    if ((Kn >= 1) && (Kn <= 2)) {
        sprintf(prm, "K%.1d", Kn);
        if (TUGrand) {                                                              // Проверяем разрешение на выдачу телеуправления
            if (cmdOn) {
                sprintf(errstr, "SCUL Send CMD: Send ON %s (ON)", prm);
                if (pLogger) pLogger->LogMessage(errstr, LOG_WRK3);
                res = SendCMD("ON", prm);
            } else {
                sprintf(errstr, "SCUL Send CMD: Send OFF %s (OFF)", prm);
                if (pLogger) pLogger->LogMessage(errstr, LOG_WRK3);
                res = SendCMD("OFF", prm);
            }
        } else {
            if (cmdOn) 
                sprintf(errstr, "SCUL Send CMD: No grant for Send ON %s (ON)", prm);
            else
                sprintf(errstr, "SCUL Send CMD: No grant for Send OFF %s (OFF)", prm);
            if (pLogger) pLogger->LogMessage(errstr, LOG_WRK3);
            res = true;
        }
    }else{
        if (pLogger) pLogger->LogMessage("SCUL Send CMD error: Wrong command param (ON/OFF)", LOG_ERR3);
    }
    return res;
}

//----------------------------------------------------------------------------
//  Послать команду "Установить аудиоканал"
//----------------------------------------------------------------------------
bool TPrtSCUL::SendSetAudioChan(int Chan)
{
    bool res = false;
    switch (Chan) {
    case 1:
        if (pLogger) pLogger->LogMessage("SCUL Send CMD: Set audio cage (SA KL)", LOG_WRK3);
        res = SendCMD("SA", "KL");
        break;
    case 2:
        if (pLogger) pLogger->LogMessage("SCUL Send CMD: Set audio engine room (SA MP)", LOG_WRK3);
        res = SendCMD("SA", "MP");
        break;
    default:
        if (pLogger) pLogger->LogMessage("SCUL Send CMD error: Wrong command param (SA)", LOG_ERR3);
        res = false;
    }
    return res;
}

//----------------------------------------------------------------------------
//  Послать команду "Сброс триггеров охраны и выхова"
//----------------------------------------------------------------------------
bool TPrtSCUL::SendSecureRST(void)
{
    if (pLogger) pLogger->LogMessage("SCUL Send CMD: Send triggers reset (TR)", LOG_WRK3);
    return SendCMD("TR");
}

//----------------------------------------------------------------------------
//  Послать команду "Установить время"
//----------------------------------------------------------------------------
bool TPrtSCUL::SendSetTime(int Year, int Month, int Day, int Hour, int Min, int Sec)
{
    char sp[15];
    char s1[50];
    memset(sp, 0, sizeof(sp));
    if (Year > 100) Year %= 100;
    if ((Month > 12) || (Month < 1)) goto SendSetTimeErr;
    if ((Day > 31) || (Day < 1)) goto SendSetTimeErr;
    if ((Hour > 24) || (Hour < 0)) goto SendSetTimeErr;
    if ((Min > 59) || (Min < 0)) goto SendSetTimeErr;
    if ((Sec > 59) || (Sec < 0)) goto SendSetTimeErr;
    sprintf(sp, "%02d%02d%02d%02d%02d%02d", Year, Month, Day, Hour, Min, Sec);
    sprintf(s1, "SCUL Send CMD: Send set time (SC) %s", sp);
    if (pLogger) pLogger->LogMessage(s1, LOG_WRK3);
    return SendCMD("SC",sp);

SendSetTimeErr:
    if (pLogger) pLogger->LogMessage("SCUL Send CMD error: Wrong command param (SC)", LOG_WRK3);
    return false;
}

//----------------------------------------------------------------------------
//  Послать команду "Установить время" с текущим временем в качестве параметра
//----------------------------------------------------------------------------
bool TPrtSCUL::SendSetCurrentTime(void)
{
    time_t t;
    tm tm;

    time(&t);
#ifdef WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    return SendSetTime(tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
}

//----------------------------------------------------------------------------
//  Послать команду "Сброс триггеров охраны и выхова"
//----------------------------------------------------------------------------
bool TPrtSCUL::SendTerminalRST(void)
{
    if (pLogger) pLogger->LogMessage("SCUL Send CMD: Reset terminal (RT)", LOG_WRK3);
    return SendCMD("RT");
}

