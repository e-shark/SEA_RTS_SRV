
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
        // Переключить аудиоканал на кабину
        if (Prm)
            res = SCULdrv->SendSetAudioChan(1);
        break;
    case 3:
        // Переключить аудиоканал на машинное
        if (Prm)
            res = SCULdrv->SendSetAudioChan(2);
        break;
    case 5:
        // Вкл/Выкл K1
            res = SCULdrv->SendKOnOff(1, 1 == Prm);
        break;
    case 6:
        // Вкл/Выкл K2
            res = SCULdrv->SendKOnOff(2, 1 == Prm);
        break;
    case 11:
        // Сбросить флаги сигнализации 1
        if (Prm)
            res = SCULdrv->SendSecureRST();
        break;
    case 12:
        // Сбросить флаги сигнализации 2
        if (Prm)
            res = SCULdrv->SendSecureRST();
        break;
    case 13:
        // Запрос состояния
        if (Prm)
            res = SCULdrv->SendGetStatus();
        break;
    case 20:
        // Reset терминала
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
    if (SEAFlags & 1) res |= 1 << DI_1;      // Служебное напряжение
    if (SEAFlags & (1 << 1)) res |= 1 << DI_ALARM1;      // Цепь безопасности 1
    if (SEAFlags & (1 << 2)) res |= 1 << DI_ALARM2;      // Цепь безопасности 1
    if (SEAFlags & (1 << 3)) res |= 1 << DI_2;           // Реле закрытия дверей
    if (SEAFlags & (1 << 4)) res |= 1 << DI_3;           // Реле открытия дверей
    if (SEAFlags & (1 << 5)) res |= 1 << DI_4;           // Реле точной остановки
    if (SEAFlags & (1 << 6)) res |= 1 << DI_5;           // Дверь кабины
    if (SEAFlags & (1 << 7)) res |= 1 << DI_6;           // Дверь шахты
    if (SEAFlags & (1 << 8)) res |= 1 << DI_7;           // Главный привод
    if (SEAFlags & (1 << 9)) res |= 1 << DI_8;           // Датчик 15 кг (Датчик пассажира)
    if (SEAFlags & (1 << 11)) res |= 1 << DI_INTR1;      // Охрана шахты
    if (SEAFlags & (1 << 12)) res |= 1 << DI_INTR2;      // Охрана машинного помещения
    if (SEAFlags & (1 << 13)) res |= 1 << DI_KEY2;       // Вызов из кабины лифта
    if (SEAFlags & (1 << 14)) res |= 1 << DI_KEY3;       // Вызов МП
    if (SEAFlags & (1 << 15)) res |= 1 << DI_15;         // ключ К1 (питание)
    if (SEAFlags & (1 << 16)) res |= 1 << DI_16;         // ключ К2 (освещение)
    if (SEAFlags & (1 << 17)) res |= 1 << DI_PHASEA;       // Питающая сеть

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
    struct _timeb tsmsb;    // Время создания пакета
#else
    struct timeb tsmsb;
#endif

    if (0 > (hdev = GetDevHandleBySno(RTUaddr))) return false;

    SetDevLinkStatus(hdev,1);                              // аналог v2mSetLinkStatus(RTUaddr);
    IncDevStat(hdev, 0);

    RTUTrF = 0;

    TSTag = 0;
    BlocksNum = GetDevBlocksNum(hdev);                                              // Получить кол-во блоков
    if (BlocksNum <= 0) return false;
    for (hblock = 0; hblock < BlocksNum; hblock++) {
        BType = GetBtype(hdev, hblock);                                             // Получить тип блока
        if (TS == BType) {                                                          // Если это блок TS
            TagsNum = GetBlockTagsNum(hdev, hblock);                                // Получаем кол-во тэгов в блоке
            if (0 == GetBpactime(hdev, hblock)) v2mOK = false;                      // Определеяем, что данные в v2m актуальные, а не обнуленные после сброса
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

                    if ((TSTag >= 0) && (TSTag <= 31)) {                            // Обрабатываем флаги состояний датчиков
                        if (RTUTS & (1 << TSTag)) BitVal = 1;                    // вычисляем новое состояние тэга
                        else BitVal = 0;

                        if (v2mOK) {
                            oldBitVal = GetTagD(hdev, hblock, tag);                        // получаем прошлое состояние тэга
                            if (oldBitVal != BitVal)
                                RTUTrF |= (1 << TSTag);
                        }

                    }

                    if ((TSTag >= 32) && (TSTag <= 63)) {               // Обрабатываем слово состояния контроллера
                        if (TSTag - 32 == state) BitVal = 1;
                        else BitVal = 0;
                    }

                    SetTagD(hdev, hblock, tag, BitVal);                 // Установить состояние тэга
                    TSTag++;
                }
                SetBtime(hdev, hblock, &tsms);
                SetBpactime(hdev, hblock, tsms.sec);
            }
            else TSTag += TagsNum;
        }
    }

    // Создаем отладочное сообщение об отправке пакета
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

    BlocksNum = GetDevBlocksNum(hdev);                                              // Получить кол-во блоков
    if (BlocksNum <= 0) return false;
    for (hblock = 0; hblock < BlocksNum; hblock++) {
        BType = GetBtype(hdev, hblock);                                             // Получить тип блока
        if (TU == BType) {
            TagsNum = GetBlockTagsNum(hdev, hblock);                                // Получаем кол-во тэгов в блоке
            if ((0 <= GetBlockChangeCount(hdev, hblock)) && (TUTag<32)) {           // Если есть изменения в блоке
                for (int tag = 0; tag < TagsNum; tag++) {                           // Просматриваем все тэги в блоке
                    if (IsTagDChanged(hdev, hblock, tag)) {                         // если состояние тэга изменилось
                        TagVal = GetTagD(hdev, hblock, tag);                        // получаем нынешнее состояние тэга
                        RTUCommandProcessing(TUTag, TagVal, SCULdrv);               // обрабатываем принятую команду (соответствующую установленому флагу - тэгу)
                        IncDevStat(hdev, 3);
                        ResetTagDChangeFlag(hdev, hblock, TUTag);                   // Сбрасываем триггер изменения в тэге
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
//  Просматривает все устройства поочереди, начиная с fromRTU
//  ищет такие, у которых не было связи больше threshold секунд
//  и они еще не взбадривались (таймер в нуле)
//  Возвращает номер такого КП, или -1, если таких нет
//-----------------------------------------------------------------------------
int GetRTUWithConnectTimeOut(int fromRTU, int threshold, int &OutTime)
{
    int res = -1;
    int hdev;
    int DQ;
    int rtuto;
    int rtutimer;

    DQ = GetDevsQuantity();                 // читаем сколько всего устройств
    hdev = fromRTU;
    if (hdev >= DQ) hdev = 0;
    for (int i = 0; i < DQ; i++) {          // просмотрим все устройства поочереди
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
//  Взводит таймер на КП RTUNum на TimerVal секунд
//-----------------------------------------------------------------------------
void SetRTUTimer(int RTUNum, int TimerVal)
{
    SetDevStat(RTUNum, 11, TimerVal);
}

//-----------------------------------------------------------------------------
//  Возвращает системный адрес КП с номером RTUNum
//-----------------------------------------------------------------------------
int GetRTUDevNo(int RTUNum)
{
    return GetDevSno(RTUNum);
}