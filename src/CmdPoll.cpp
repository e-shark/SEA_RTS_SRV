

#include <stdlib.h>
#include <string.h>

#include "CmdPoll.h"

//=========================================================================================================

//----------------------------------------------------------------------------
//  ������� ������ ������ PollPacket
//----------------------------------------------------------------------------
TPollPacket::TPollPacket(void)
{
    Init();
}

//----------------------------------------------------------------------------
//  ������� ������ PollPacket � �������� � ���� ������ �� ���������
//  ����������� ���� ������ �������, �������� ���������� ������.
//----------------------------------------------------------------------------
TPollPacket::TPollPacket(void *Data, int Len)
{
    Init();
    FillData(Data, Len);
    isObject = false;
}

//----------------------------------------------------------------------------
//  ������� ������ PollPacket � �������� � ���� ��������� �� ������ 
//  � ���������, �� ��������� �������� ������
//  ����������� � ������, ����� ������ - ������� ������.
//----------------------------------------------------------------------------
TPollPacket::TPollPacket(void *Data, TPcketDestroyer pDestroyer)
{
    Init();
    pData = Data;
    Destroyer = pDestroyer;
    isObject = true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void TPollPacket::Init(void)
{
    isObject = false;
    pData = NULL;
    DataLen = 0;
    Destroyer = NULL;
#ifdef WIN32
    _ftime(&CreatTime);     // ���������� ����� �������� ������
#else
    ftime(&CreatTime);
#endif
    LifeTime = 10000;        // ����� ����� ������ �� ���������
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
TPollPacket::~TPollPacket()
{
    FreeData();
}

//----------------------------------------------------------------------------
//  �������� ������ ��� ������ � �������� ������ ������ �� ���������
//  � ���������� ������.
//  ����������� ���� ��������� ������ ����� ������ �����������
//  ���� ������ ����� ����� ������� ���������, �� ��� ��������� ���-��
//  � � PollPacket ������������ ��������� �� ��� (pData) (� �� ����� (DataLen)), 
//  � ����������� ��������� ��������� ������ (Destroyer)
//----------------------------------------------------------------------------
bool TPollPacket::FillData(void *Data, int Len)
{
    bool res = true;
    if ((Data != NULL) && (Len > 0)) {
        if (pData) free(pData);
        isObject = false;
        pData = malloc(Len);
        try {
            memcpy(pData, Data, Len);
        }
        catch (...)
        {
            res = false;
        }
        if (res)
            DataLen = Len;
        else {
            free(pData);
            DataLen = 0;
        }
    }
    return res;
}

//----------------------------------------------------------------------------
//  ������� ������
//  ���� ������ - ������, �������, ���� �� Destroyer ��� ����.
//  ���� ���� Destroyer, ���������� ��, ���� ���, ������ delete. 
//  ���� ������ - ������, ������ ����������� ������.
//----------------------------------------------------------------------------
void TPollPacket::FreeData(void)
{
    if (!pData) return;             // ���� ���� - ���� ����
    if (isObject) {
        if (Destroyer)
            Destroyer(pData);
        else
            delete pData;
    } else
        if (pData) free(pData);
    pData = NULL;
    DataLen = 0;
}

//----------------------------------------------------------------------------
//  ���������, �� ����������� �� ����� ����� � ������.
//  ���������� true, ���� ����� ����� �����������.
//  ���� LifeTime == 0, �� ����� � ����������� �������� ��������.
//----------------------------------------------------------------------------
bool TPollPacket::CheckLifeTimePasse(void)
{
    bool res = false;
    unsigned long elapsettime;
#ifdef WIN32
    struct _timeb NowTime;
#else
    struct timeb NowTime;
#endif

    if (LifeTime > 0) {
#ifdef WIN32
        _ftime(&NowTime);
#else
        ftime(&NowTime);
#endif
        elapsettime = (CreatTime.time * 1000 + CreatTime.millitm) - (NowTime.time * 1000 + NowTime.millitm);
        if (elapsettime > LifeTime) res = true;
    }
    return res;
}

//=========================================================================================================


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
TCommandPoll::TCommandPoll(void)
{
    PacketsCount = 0;
    for (int i = 0; i < PacketStackDepth - 1; i++) {
        Packets[i] = NULL;;
    }
    osInitThreadMutex(&hLPMutex);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
TCommandPoll::~TCommandPoll()
{
    osLockThreadMutex(&hLPMutex);
    for (int i = 0; i < PacketStackDepth - 1; i++) {
        if (Packets[i]) delete Packets[i];
    }
    PacketsCount = 0;
    //osUnlockThreadMutex(&hLPMutex);
    osDestroyThreadMutex(&hLPMutex);
}
                                     
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool TCommandPoll::LockPoll(void)
{
    return 0 == osLockThreadMutex(&hLPMutex);
    /*
    bool res = false;
    int rcount = 0;
    do {
        res = 0 == osTryLockThreadMutex(&hLPMutex);
        if (!res) {
            osSleep(50);
            rcount++;
        }
    } while (!res && (rcount < 20));
    return res;
    */
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void TCommandPoll::UnLockPoll(void)
{
    osUnlockThreadMutex(&hLPMutex);
}

//----------------------------------------------------------------------------
//  ���������� ����� ����� � ���� �������
//  �������� � ���� ������ �� ���������
//  (���� ���� ����������, ������� �� ���� ������ (����� ������) �����)
//----------------------------------------------------------------------------
int TCommandPoll::PushPacket(void *Data, int Len)
{
    TPollPacket *pack;
    if (LockPoll()) {
        pack = new TPollPacket(Data, Len);
        if (PacketStackDepth == PacketsCount) {      // ���� ������� �������������
            DeletePacket(0);
        }
        Packets[PacketsCount] = pack;
        PacketsCount++;
        UnLockPoll();
        return PacketsCount - 1;
    }
    else
        return -1;
}

//----------------------------------------------------------------------------
//  ���������� ����� ����� � ���� �������
//  ������� �����, ���������� � ���� ����� ������ �� ������ Data � ������ �� ������� �������� ������ Destroyer.
//  (���� ���� ����������, ������� �� ���� ������ (����� ������) �����)
//  Destroyer ���������� ���� ���� ������� ������, ��� �������� ������.
//  ���� Destroyer �� ������ (�� ��������� NULL), ���������� ����������� ��� ������� delete.
//----------------------------------------------------------------------------
int TCommandPoll::PushPacket(void *Data, TPcketDestroyer Destroyer)
{
    TPollPacket *pack;
    if (LockPoll()) {
        pack = new TPollPacket();
        pack->pData = Data;
        pack->Destroyer = Destroyer;
        if (PacketStackDepth == PacketsCount) {      // ���� ������� �������������
            DeletePacket(0);
        }
        Packets[PacketsCount] = pack;
        PacketsCount++;
        UnLockPoll();
        return PacketsCount - 1;
    }
    else
        return -1;
}

//----------------------------------------------------------------------------
//  ������� ����� � ��������� ��������, ����������� ������ �������� � ������ �������
//----------------------------------------------------------------------------
void TCommandPoll::DeletePacket(int index)
{
    if (index < PacketsCount) {
        if (Packets[index]) {
            delete Packets[index];
        }
        PacketsCount--;
        for (int i = index; i < PacketStackDepth - 2; i++) {
            Packets[i] = Packets[i + 1];
        }
    }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int TCommandPoll::GetPacketsCount(void)
{
    return PacketsCount;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
TPollPacket * TCommandPoll::GetPacket(int index)
{
    TPollPacket* res = NULL;
    if (LockPoll()) {

        UnLockPoll();
    }
    return res;
}

//----------------------------------------------------------------------------
//  ������� �� ����� ����� � ���������� ��������� �� ����.
//  ��������� �� ����� ����������� �� �����, �� ��� ����� � ��� ������ �� ���������.
//  ���� ������ ������ ���������� ���������� ���������.
//  (�.�. ���������� ��������� ������ ������� delete ��� ����� ������)
//----------------------------------------------------------------------------
TPollPacket* TCommandPoll::PopPacket(void)
{
    TPollPacket* res = NULL;
    if (LockPoll()) {
        if (PacketsCount)
            if (Packets[0]) {
                res = Packets[0];
                PacketsCount--;
                for (int i = 0; i < PacketStackDepth - 2; i++) {
                    Packets[i] = Packets[i + 1];
                }
            }
        UnLockPoll();
    }
    return res;
}

//----------------------------------------------------------------------------
//  ������� �� ������� ������ � �������� "�������� ��������".
//  ���������� ���������� ��������� �� ����� ���������� �������.
//----------------------------------------------------------------------------
int TCommandPoll::CheckForDated(void)
{
    int res = 0;
    if (LockPoll()) {
        for (int i = 0; i < PacketsCount - 1; i++) {
            if (Packets[i])
                if ((Packets[i])->CheckLifeTimePasse()) {
                    DeletePacket(i);
                    res++;
                }
        }
        UnLockPoll();
    }
    return res;
}




