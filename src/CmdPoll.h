#ifndef CmdPollH
#define CmdPollH
//----------------------------------------------------------------------------
#include <sys/timeb.h>  

#include "oswr.h"
//----------------------------------------------------------------------------
#define PacketStackDepth 32
//----------------------------------------------------------------------------
typedef void(*TPcketDestroyer)(void *Data);

//----------------------------------------------------------------------------
class TPollPacket
{
protected:
    bool isObject;
#ifdef WIN32
    struct _timeb CreatTime;    // ����� �������� ������
#else
    struct timeb CreatTime;
#endif
    void Init(void);
public:
    void *pData;                // ��������� �� ������
    int DataLen;                // ����� ������ � ������
    unsigned long LifeTime;     // ����� ����� ������ � ������������

    TPollPacket(void);          
    TPollPacket(void *Data, int Len);
    TPollPacket(void *Data, TPcketDestroyer pDestroyer);
    ~TPollPacket();

    bool CheckLifeTimePasse(void);
    bool FillData(void *Data, int Len); 
    void FreeData(void);
    TPcketDestroyer Destroyer;
};

//----------------------------------------------------------------------------
class TCommandPoll
{
private:
    pthread_mutex_t hLPMutex;

protected:
    TPollPacket *Packets[PacketStackDepth];
    int PacketsCount;
public:
    TCommandPoll(void);
    ~TCommandPoll();

    bool LockPoll(void);
    void UnLockPoll(void);
    // � ����� ��������� ���� �������� � �������������� LockPoll/UnLockPoll
    void DeletePacket(int index);
    TPollPacket * GetPacket(int index);
    // --------------------------------------------------------------------

    int GetPacketsCount(void);
    int PushPacket(void *Data, int Len);
    int PushPacket(void *Data, TPcketDestroyer Destroyer = NULL);
    int CheckForDated(void);
    TPollPacket* PopPacket(void);
};

#endif