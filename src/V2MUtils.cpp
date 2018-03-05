
#include "V2MUtils.h"


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int GetConnectTimeOutRTU(int from, int threshold, int &OutTime)
{
    int res = -1;
    int hdev;
    int DQ;
    int rtuto;
    int rtutimer;
    int TOSET = 60 * 3;

    DQ = GetDevsQuantity();
    for (hdev = 0; hdev < DQ; hdev++) {
        rtuto = GetDevElapsedTime(hdev);
        rtutimer = GetDevStat(hdev, 11);
        if (rtuto > TOSET) {
            res = hdev;
            OutTime = rtuto;
        }
    }
    return res;
}




