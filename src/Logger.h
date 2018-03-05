#ifndef LoggerH
#define LoggerH

#include <string>
#include "oswr.h"

//----------------------------------------------------------------------------
#define MaxLogMessageLen 1024       // Максимальная длина сообщения

//  Маски логируемых сообщений
#define LOG_WRK1     0x0001         // Основные этапы рабочего алгоритма
#define LOG_WRK2     0x0002         // Шаги основных этапов
#define LOG_WRK3     0x0004         // Подробности алгоритма

#define LOG_ERR1     0x0010         // Критические ошибки      (могут привести к аварийному завершению приложения)
#define LOG_ERR2     0x0020         // Неритические ошибки     (могу привести к аварийному завершению текущей процедуры, команды. приложение при этом продолжит работать)
#define LOG_ERR3     0x0040         // Мелкие "спотыкания"     (мелкая ошибка, которая ожидалась и правильно обрабатывается, при этом не влияет на работу приложения в целом)

#define LOG_DBG1     0x0100         // Основные этапы отладки
#define LOG_DBG2     0x0200         // Доплнительная информация
#define LOG_DBG3     0x0400         // Специальная отладочная информация

//----------------------------------------------------------------------------
class TLogger
{
private:
protected:
    pthread_mutex_t hLogMutex;
    int LogFileMask;
    int LogConMask;
    int LogDBMask;

    int(*pDBLog)(std::string Message);

    bool LogToFile(std::string Message);
    bool LogToConsole(std::string Message);
    bool LogToDB(std::string Message);

    std::string TLogger::MessageStuffing(std::string Message);
public:
    TLogger();
    ~TLogger();
    std::string LogFilePath;                // Путь, куда писать логи
    std::string LogFilePrefix;              // Первая часть имени .log файла (вторя часть - это дата)
    std::string MesagePrefix;               // Префикс, который добавляется в начало каждого сообщения

    bool SetLogFilePath(char *FileName);
    bool SetLogFilePrefix(char *FileName);
    bool SetDBLogger(int(*p)(std::string Message));
    void SetLogMask(int FileMask, int ConMask, int DBMask);

    bool LogMessage(std::string Message, int Level = LOG_WRK1);
    bool LogMessageWoDB(std::string Message, int Level = LOG_WRK1);
    bool LogData(std::string Header, void * Ptr, int len, int Mask);
};

#endif
