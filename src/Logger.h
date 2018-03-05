#ifndef LoggerH
#define LoggerH

#include <string>
#include "oswr.h"

//----------------------------------------------------------------------------
#define MaxLogMessageLen 1024       // ������������ ����� ���������

//  ����� ���������� ���������
#define LOG_WRK1     0x0001         // �������� ����� �������� ���������
#define LOG_WRK2     0x0002         // ���� �������� ������
#define LOG_WRK3     0x0004         // ����������� ���������

#define LOG_ERR1     0x0010         // ����������� ������      (����� �������� � ���������� ���������� ����������)
#define LOG_ERR2     0x0020         // ������������ ������     (���� �������� � ���������� ���������� ������� ���������, �������. ���������� ��� ���� ��������� ��������)
#define LOG_ERR3     0x0040         // ������ "����������"     (������ ������, ������� ��������� � ��������� ��������������, ��� ���� �� ������ �� ������ ���������� � �����)

#define LOG_DBG1     0x0100         // �������� ����� �������
#define LOG_DBG2     0x0200         // ������������� ����������
#define LOG_DBG3     0x0400         // ����������� ���������� ����������

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
    std::string LogFilePath;                // ����, ���� ������ ����
    std::string LogFilePrefix;              // ������ ����� ����� .log ����� (����� ����� - ��� ����)
    std::string MesagePrefix;               // �������, ������� ����������� � ������ ������� ���������

    bool SetLogFilePath(char *FileName);
    bool SetLogFilePrefix(char *FileName);
    bool SetDBLogger(int(*p)(std::string Message));
    void SetLogMask(int FileMask, int ConMask, int DBMask);

    bool LogMessage(std::string Message, int Level = LOG_WRK1);
    bool LogMessageWoDB(std::string Message, int Level = LOG_WRK1);
    bool LogData(std::string Header, void * Ptr, int len, int Mask);
};

#endif
