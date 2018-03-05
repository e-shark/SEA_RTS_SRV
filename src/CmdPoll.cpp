

#include <stdlib.h>
#include <string.h>

#include "CmdPoll.h"

//=========================================================================================================

//----------------------------------------------------------------------------
//  Создает пустой объект PollPacket
//----------------------------------------------------------------------------
TPollPacket::TPollPacket(void)
{
    Init();
}

//----------------------------------------------------------------------------
//  Создает объект PollPacket и копирует в него данные из источника
//  Применяется если данные простые, например символьная строка.
//----------------------------------------------------------------------------
TPollPacket::TPollPacket(void *Data, int Len)
{
    Init();
    FillData(Data, Len);
    isObject = false;
}

//----------------------------------------------------------------------------
//  Создает объект PollPacket и копирует в него указатель на данные 
//  и указатель, на процедуру удаления данных
//  Применяется в случае, когда данные - сложный объект.
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
    _ftime(&CreatTime);     // Инициируем время создания пакета
#else
    ftime(&CreatTime);
#endif
    LifeTime = 10000;        // Время жизни пакета по умолчанию
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
TPollPacket::~TPollPacket()
{
    FreeData();
}

//----------------------------------------------------------------------------
//  Выделяет память под данные и копирует массив данных из источника
//  в выделенную облать.
//  Применяется если структуру данных можно просто скопировать
//  Если данные имеют более сложную структуру, то они создаются где-то
//  а в PollPacket записывается указатель на них (pData) (и их длину (DataLen)), 
//  и указывается процедура удаляющая данные (Destroyer)
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
//  Удаляет данные
//  Если данные - объект, смотрим, есть ли Destroyer для него.
//  Если есть Destroyer, вызывается он, если нет, просто delete. 
//  Если данные - строка, просто освобождаем память.
//----------------------------------------------------------------------------
void TPollPacket::FreeData(void)
{
    if (!pData) return;             // нету тела - нету дела
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
//  Проверяет, не закончилось ли аремя жизни у пакета.
//  Возвращает true, если время жизни закончилось.
//  Если LifeTime == 0, то пакет с бесконечным временем хранения.
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
//  Запихивает новый пакет в стек пакетов
//  Копирует в него данные из источника
//  (если стек переаолнен, удаляет из него первый (самый старый) пакет)
//----------------------------------------------------------------------------
int TCommandPoll::PushPacket(void *Data, int Len)
{
    TPollPacket *pack;
    if (LockPoll()) {
        pack = new TPollPacket(Data, Len);
        if (PacketStackDepth == PacketsCount) {      // Если бабушка переполнилась
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
//  Запихивает новый пакет в стек пакетов
//  Создает пакет, записывает в этот пакет ссылку на дынные Data и ссылку на функцию удаления данных Destroyer.
//  (если стек переаолнен, удаляет из него первый (самый старый) пакет)
//  Destroyer вызывается если надо удалить данные, при удалении пакета.
//  Если Destroyer не указан (по умолчанию NULL), вызывается стандартный для объекта delete.
//----------------------------------------------------------------------------
int TCommandPoll::PushPacket(void *Data, TPcketDestroyer Destroyer)
{
    TPollPacket *pack;
    if (LockPoll()) {
        pack = new TPollPacket();
        pack->pData = Data;
        pack->Destroyer = Destroyer;
        if (PacketStackDepth == PacketsCount) {      // Если бабушка переполнилась
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
//  Удаляет пакет с указанным индексом, последующие пакеты сдвигает к началу массива
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
//  Достает из стека пакет и возвращает указатель на него.
//  Указатель на пакет извлекается из стека, но сам пакет и его данные не удаляется.
//  Этим теперь должна заниматься вызывающая программа.
//  (Т.е. вызывающая программа должна вызвать delete для этого пакета)
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
//  Удаляет из очереди пакеты с истекшим "временем хранения".
//  Возвращает количеству удаленных из стека устаревших пакетов.
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




