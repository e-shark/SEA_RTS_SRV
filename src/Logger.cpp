/*****************************************************************************
*   Модуль логрования
*   Диденко А.В.    20.I.2017
*****************************************************************************/

#include <time.h>
#include <sys/timeb.h>

#include "Logger.h"
#include "utils.h"

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
TLogger::TLogger()
{
    LogFilePath = "";
    LogFilePrefix = "LOG";
    LogFileMask = 0x0777;  //логировать все
    LogConMask = 0x0777;
    LogDBMask = 0x0777;
    pDBLog = NULL;
    osInitThreadMutex(&hLogMutex);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
TLogger::~TLogger()
{
    osDestroyThreadMutex(&hLogMutex);
    pDBLog = NULL;
}

//----------------------------------------------------------------------------
//  Установить имя файла логирования
//----------------------------------------------------------------------------
void TLogger::SetLogMask(int FileMask, int ConMask, int DBMask)
{
    LogFileMask = 0x0777 & FileMask;
    LogConMask = 0x0777 & ConMask;
    LogDBMask = 0x0777 & DBMask;
}

//----------------------------------------------------------------------------
//  Установить имя файла логирования
//----------------------------------------------------------------------------
bool TLogger::SetLogFilePath(char * FilePath)
{
    if (strlen(FilePath) > 250) return false;

    LogFilePath = FilePath;
    return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool TLogger::SetLogFilePrefix(char *Prefix)
{
    if (strlen(Prefix) > 50) return false;

    LogFilePrefix = Prefix;
    return true;
}
//----------------------------------------------------------------------------
//  Установить объект, товечающий за логирование в БД
//----------------------------------------------------------------------------
bool TLogger::SetDBLogger(int(*p)(std::string Message))
{
    bool res = true;
    try {
        if (p) pDBLog = p;
    }
    catch (...)
    {
        res = false;
    }
    return res;
}

//-----------------------------------------------------------------------------
//  Записать лог сообщение в файл
//-----------------------------------------------------------------------------
bool TLogger::LogToFile(std::string Message)
{
    char fname[256];
    time_t t;
    tm*tm;
    timeb tb;

    if (Message.length() == 0) return true;

    osLockThreadMutex(&hLogMutex);

    time(&t);
    tm = localtime(&t);
    sprintf(fname, "%s%s%02d%02d%02d.log", LogFilePath.c_str(), LogFilePrefix.c_str(), tm->tm_mday, tm->tm_mon + 1, tm->tm_year - 100);

    FILE *hfile = fopen(fname, "a");
    if (hfile == NULL) return false;

    ftime(&tb);
    tm = localtime(&tb.time);

    fprintf(hfile, "%s %02d:%02d:%02d.%03d %s\n", MesagePrefix.c_str(), tm->tm_hour, tm->tm_min, tm->tm_sec, tb.millitm, Message.c_str());

    fclose(hfile);

    osUnlockThreadMutex(&hLogMutex);

    return true;
}

//-----------------------------------------------------------------------------
//  Вывести лог сообщение в консоль
//-----------------------------------------------------------------------------
bool TLogger::LogToConsole(std::string Message)
{ 
    std::string fullMessage;
    bool res = false;
    if (Message.length() == 0) return true;
    fullMessage = MesagePrefix;
    fullMessage += Message;

    osLockThreadMutex(&hLogMutex);

    int n = fwrite(fullMessage.c_str(), 1, fullMessage.length(), stdout);
    if (!ferror(stdout)) {
        fputc('\n', stdout);
        res = true;
    }else
        res = false;

    osUnlockThreadMutex(&hLogMutex);

    return res;
}

//-----------------------------------------------------------------------------
//  Записать лог сообщение в БД
//-----------------------------------------------------------------------------
bool TLogger::LogToDB(std::string Message)
{
    std::string fullMessage;

    if (Message.length() == 0) return true;
    fullMessage = MesagePrefix;
    fullMessage += Message;
    if (pDBLog) {
        if (0 == pDBLog(fullMessage)) return true;
        else return false;
    }else 
        return false;
}

//-----------------------------------------------------------------------------
//  Заменяет управляющие смиволы в строке на их названия
//-----------------------------------------------------------------------------
std::string TLogger::MessageStuffing(std::string Message)
{
    std::string MSres;
    char *pC;
    MSres = "";
    pC = (char*) Message.c_str();
    for (int i = 0; i < Message.size(); i++) {
        if (*pC < 20) {
            switch (*pC) {
            case '\r': MSres += "\\r"; break;
            case '\n': MSres += "\\n"; break;
            case '\t': MSres += "\\t"; break;
            case '\b': MSres += "\\b"; break;
            default:
                MSres += '{';
                MSres += IntToStr(*pC);
                MSres += '}';
            }
        } else {
            MSres += *pC;
        }
        pC++;
    } // from
    return MSres;
}

//-----------------------------------------------------------------------------
//  Записывает log сообщение (во все места назначения)
//  в зависимости от флагов маски и настроек разрешений
//  Строка сообщения обязана заканчиваться #0
//-----------------------------------------------------------------------------
bool TLogger::LogMessage(std::string Message, int Mask)
{
    std::string MS;
    if (Message.length() == 0) return true;
    MS = MessageStuffing(Message);
    try {
        if (LogFileMask & Mask) LogToFile(MS);
        if (LogConMask & Mask) LogToConsole(MS);
        if (LogDBMask & Mask) LogToDB(MS);
    }
    catch(...)
    {
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
//  То-же, что LogMessage, только не пишет в базу
//  Функция нужна для логирования ошибок БД
//-----------------------------------------------------------------------------
bool TLogger::LogMessageWoDB(std::string Message, int Mask)
{
    std::string MS;
    if (Message.length() == 0) return true;
    MS = MessageStuffing(Message);
    try {
        if (LogFileMask & Mask) LogToFile(MS);
        if (LogConMask & Mask) LogToConsole(MS);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
//  Записывает в log дамп данных в формате
//  Header [len]: XX XX XX XX ...
//-----------------------------------------------------------------------------
bool TLogger::LogData(std::string Header, void * Ptr, int len, int Mask)
{
    std::string Message;
    char s[10];

    Message = MesagePrefix;
    Message += Header;
    sprintf(s, "[%d]:", len);
    Message += s;
    for (int i = 0; i < len; i++) {
        sprintf(s, " %.2X ", ((unsigned char*)Ptr)[i]);
        Message += s;
    }
    return LogMessage(Message, Mask);
}

