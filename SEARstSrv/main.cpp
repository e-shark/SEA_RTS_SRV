/*------------------------------------------------------------------------------
*	24 января 2017г.
*	Диденко А.В.
*	Служба удаленного сброса контроллеров лифтов Sea SCUL.
*----------------------------------------------------------------------------*/


#include "main.h"
#include "cutils.h"
#include "Logger.h"
#include "DBUtils.h"
#include "V2MConnect.h"
#include "ATCommander.h"



bool bMainThreadStopped = false;            // set to true to stop Main thread
bool bServiceThreadStopped = false;         // set to true to stop Main thread
char ExecPath[256] = "";
char LogPath[256] = "";
char IniFileName[256] = "";
int  gLOG_FILE_MASK = 0;
int  gLOG_CON_MASK = 0;
int  gLOG_DB_MASK = 0;
HANDLE hMainThread = NULL;                  // Handle to thread implementing MainForm cycle
HANDLE hServiceThread = NULL;               // Handle to thread implementing MainForm cycle
TLogger Logger;
int TimeOutThreshold = 60 * 10;             // Через сколько времени (в секундах) от последго событияй связи пытаться сбросить девайс
int RstAttemptInterval = 60 * 5;            // С каким периодом (в секундах) повторять попытки сбросить девайс
int ConnectionTimeOut = 60 * 50;            // Время (в секундах) Ожидания соединения при дозвоне

int COMPORT = 1;
int COMBAUD = 9600;

//-----------------------------------------------------------------------------
//  Service CYCLE
//-----------------------------------------------------------------------------
DWORD ServiceCycleThread(LPDWORD lpdwParam)
{
    Logger.LogMessage("start service cycle.", LOG_WRK2);
    v2mSetLogger(&Logger);

    while (!bServiceThreadStopped) {
        v2mDynamicConnection();             // Обслуживаем v2m драйвер
        osSleep(1000);
    }

    Logger.LogMessage("service cycle stoped.", LOG_WRK2);
    ExitThread(0);
    return 0;
}


//-----------------------------------------------------------------------------
//  MAIN CYCLE
//-----------------------------------------------------------------------------
DWORD MainCycleThread(LPDWORD lpdwParam)
{
    bool isdbconnected = false;
    char phone[50];
    int lastrtu= -1;
    int rtu=-1;
    int rtuto=0;
    int dbres;
    int rtuAddr;
    char msgstr[512];
    TATCommander ATCmder;
    ATCmder.SetLogger(&Logger);
    ATCmder.SetComNum(COMPORT);
    ATCmder.SetComBaud(COMBAUD);

    Logger.LogMessage("start main cycle.", LOG_WRK2);

    try {
        while (!bMainThreadStopped) {
            rtu = GetRTUWithConnectTimeOut(lastrtu, TimeOutThreshold, rtuto);
            if (rtu >= 0) {
                lastrtu = rtu;
                rtuAddr = GetRTUDevNo(rtu);
                dbres = GetPhoneForRTU(rtuAddr, phone, 49);
                if (0 == dbres) {
                    sprintf(msgstr, "rtur:%d tout %d sec try reset by %s", rtuAddr, rtuto, phone);
                    Logger.LogMessage(msgstr, LOG_WRK3);
                    if (!ATCmder.SEATryRemoteReset(phone, ConnectionTimeOut)) {
                        sprintf(msgstr, "rtur:%d attempt failed!", rtuAddr);
                        Logger.LogMessage(msgstr, LOG_WRK3);
                    }
                    SetRTUTimer(rtu, RstAttemptInterval);
                    osSleep(10000);
                } else {
                    sprintf(msgstr, "can't finde phone for rtuaddr %d (rtu No %d)", rtuAddr, rtu);
                    Logger.LogMessage(msgstr, LOG_ERR1);
                    osSleep(5000);
                }
            } else osSleep(1000);
            
        } //whyle
    } //try
    catch (...) {
        Logger.LogMessage("Unexpected error! Thread aborted.", LOG_ERR1);
    }

    CloseDB();
    bServiceThreadStopped = true;
    Logger.LogMessage("service cycle stoped.", LOG_WRK2);
    ExitThread(0);
    return 0;

}

//-----------------------------------------------------------------------------
//	Старт сервиса 
//	Возвращаемое значение:
//		>0 - сервис запущен нормально
//		<=0 - ошибка ( - код ошибки)
//-----------------------------------------------------------------------------
int SeaSculRstSrv_Start(int argc, char **argv)
{
    int ErrCod = 0;
    DWORD dwThreadId;
    char s[512];
    char *pC;
    int dbretcode;

    // Определяем путь к запускаемому файлу 
#ifdef _LINUX_
    strncpy(ExecPath, argv[0], sizeof(ExecPath) - 1);
    pC = strrchr(ExecPath, '/');
#else
    if (GetModuleFileName(NULL, ExecPath, sizeof(ExecPath)) == 0)
        memset(ExecPath, 0, sizeof(ExecPath));
    pC = strrchr(ExecPath, '\\');
#endif
    if (pC) {
        pC++;
        *(pC) = 0;
    }
    else
        memset(ExecPath, 0, sizeof(ExecPath));

    sprintf(IniFileName, "%sSEASrv.ini", ExecPath);                                                         // Вычисляем имя .ini файла

    cuGetPrivateProfileString("INIT", "RTUDBSTRING", "Elevators_ds", s, sizeof(s), IniFileName);            // Логический адрес девайса диспетчерской (от этого имени посылаются пакеты лифтовым контроллерам)
    strtrim(s);
    sprintf(DBCONNSTRING, "%s", s);

    TimeOutThreshold = cuGetPrivateProfileInt("RSTSRV", "TimeOutThreshold", 60 * 10, IniFileName);          // Через сколько времени (в секундах) от последго событияй связи пытаться сбросить девайс
    RstAttemptInterval = cuGetPrivateProfileInt("RSTSRV", "RstAttemptInterval", 60 * 5, IniFileName);       // С каким периодом (в секундах) повторять попытки сбросить девайс
    COMPORT = cuGetPrivateProfileInt("RSTSRV", "COMPORT", 1, IniFileName);                                  // Номер COM порта, к которому подключен модем для voice звонка
    COMBAUD = cuGetPrivateProfileInt("RSTSRV", "COMBAUD", 9600, IniFileName);                               // Скорость обмена по COM порту

    int mask;
    int d1, d2, d3;
    mask = cuGetPrivateProfileInt("RSTSRV", "LogFileMask", 0, IniFileName);                                 // По умолчанию 0 - логирование отключено
    d3 = mask % 10;
    d2 = (mask / 10) % 10;
    d1 = (mask / 100) % 10;
    gLOG_FILE_MASK = (d1 << 8) | (d2 << 4) | d3;

    mask = cuGetPrivateProfileInt("RSTSRV", "LogConMask", 0, IniFileName);
    d3 = mask % 10;
    d2 = (mask / 10) % 10;
    d1 = (mask / 100) % 10;
    gLOG_CON_MASK = (d1 << 8) | (d2 << 4) | d3;

    mask = 0; // = cuGetPrivateProfileInt("INIT", "LogDBMask", 0, IniFileName);
    d3 = mask % 10;
    d2 = (mask / 10) % 10;
    d1 = (mask / 100) % 10;
    gLOG_DB_MASK = (d1 << 8) | (d2 << 4) | d3;

    Logger.SetLogMask(gLOG_FILE_MASK, gLOG_CON_MASK, gLOG_DB_MASK);

    cuGetPrivateProfileString("INIT", "LogPath", ExecPath, s, sizeof(s), IniFileName);                      // Путь, где будет записан LOG файл
    strtrim(s);
    sprintf(LogPath, "%s", s);

#define LogInSingleDir
#ifdef LogInSingleDir
#ifdef _LINUX_
    sprintf(s, "Create Dir: %sLog/", LogPath);
    LogMessage(s, ML_DBG2);
    //sprintf(s, "%sLog/", ExecPath);
    mkdir(LogPath);
#else
    sprintf(s, "Create Dir: %sLog\\", LogPath);
    //LogMessage(s, ML_DBG2);
    //sprintf(s, "%sLog\\", ExecPath);
    CreateDirectory(LogPath, NULL);
#endif
    //SetLogPath(s);                                  // Указываем, куда писать логи
#endif
    Logger.SetLogFilePath(LogPath);                   // Указываем, куда писать логи
    Logger.SetLogFilePrefix("RST");

    Logger.LogMessage("===============================================================================", LOG_WRK1);
    sprintf(s, "Starting Rst Service (build %s %s)", __DATE__, __TIME__);
    Logger.LogMessage(s, LOG_WRK1);
    sprintf(s, "ExecPath: %s", ExecPath);
    Logger.LogMessage(s, LOG_WRK1);

    ErrCod = -1;
    dbretcode = TestDb();
    if (0 == dbretcode) {
        sprintf(s, "Connected to %s OK", (unsigned char*)DBCONNSTRING);
        Logger.LogMessage(s, LOG_WRK2);

        sprintf(s, "TimeOutThreshold: %d", TimeOutThreshold);
        Logger.LogMessage(s, LOG_WRK1);

        sprintf(s, "RstAttemptInterval: %d", RstAttemptInterval);
        Logger.LogMessage(s, LOG_WRK1);


        bServiceThreadStopped = false;
        bMainThreadStopped = false;
        // cсоздаем основную нитку 
        hServiceThread = CreateThread(
            NULL,                                     /* no security attributes */
            0,                                        /* use default stack size */
            (LPTHREAD_START_ROUTINE)ServiceCycleThread, /* thread function */
            NULL,                                     /* argument to thread function */
            CREATE_SUSPENDED,                         /* creation flags */
            &dwThreadId);
        if (hServiceThread) {
            ErrCod = -2;
            // запускаем созданную нитку
            if (ResumeThread(hServiceThread) == 1)
                ErrCod = 1;
            else
                ErrCod = -3;
        }

        if (1 == ErrCod) {
            ErrCod = -4;
            // cсоздаем основную нитку 
            hMainThread = CreateThread(
                NULL,                                     /* no security attributes */
                0,                                        /* use default stack size */
                (LPTHREAD_START_ROUTINE)MainCycleThread, /* thread function */
                NULL,                                     /* argument to thread function */
                CREATE_SUSPENDED,                         /* creation flags */
                &dwThreadId);
            if (hMainThread) {
                ErrCod = -5;
                // запускаем созданную нитку
                if (ResumeThread(hMainThread) == 1)
                    ErrCod = 2;
                else
                    ErrCod = -6;
            }
        }
    }
    else {
        sprintf(s, "Error %d while connecting to DB %s", dbretcode, (unsigned char*)DBCONNSTRING);
        Logger.LogMessage(s, LOG_WRK1);
        ErrCod = -7;
    }

    return ErrCod;
}


//-----------------------------------------------------------------------------
//	Останов сервиса 
//-----------------------------------------------------------------------------
void SeaSculRstSrv_Stop()
{
    Logger.LogMessage("stoping SEA Rst Service...", LOG_WRK1);
    bMainThreadStopped = true;
    osSleep(1500);
    bServiceThreadStopped = true;
    osSleep(1500);
    Logger.LogMessage("SEA Rst Service stoped.", LOG_ERR1);
}


