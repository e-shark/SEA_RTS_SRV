

#include <stdio.h>

#include "DBUtils.h"

#include "oswr.h"       // Depends on the library of simple OS wrappers (oswrw.cpp|oswrx.cpp/oswr.h)
#include "osodbc.h"     // Depends on the library of simple OS odbc wrappers (osodbc.cpp/osodbc.h)

SQLHENV henv;   // Handle of odbc environment
SQLHDBC hdbc;   // Handle of odbc connection
bool isdbconnected = false;

#define MUTEXDBREGNAME "LiftDBUtilsUnitMutex"     // Mutex for arbitration the access to alarm_registry table
void* hDbRegMx; // Handle of above Mutex

char DBCONNSTRING[256] = "";

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool connectDb(char* ds_name)
{
    int retcode;
    if (!isdbconnected) {
        if (0 > (retcode = osOdbcInit(&henv)))                   return false;
        if (0 > (retcode = osOdbcConnect(henv, (SQLCHAR*)ds_name, &hdbc))) return false;
        isdbconnected = true;
    }
    return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int TestDb()
{
    int retcode;
    if (0 > (retcode = osOdbcInit(&henv)))                   return retcode;
    if (0 > (retcode = osOdbcConnect(henv, (SQLCHAR*)DBCONNSTRING, &hdbc))) return retcode;
    retcode = osCreateMutex(&hDbRegMx, MUTEXDBREGNAME);
    return 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool disconnectDb()
{
    int retcode;
    isdbconnected = false;
    if (0 > (retcode = osOdbcDisconnect(hdbc)))      return false;
    if (0 > (osOdbcFree(henv)))                      return false;
    return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int GetPhoneForRTU(int RtuNo, char* Phone, int buflen)
{
    int res = 0;
    char sqlreq[100];
    char resphone[50];
    int dbres = 0;
    SQLHSTMT hstmt;
    SQLRETURN retcode;
    SQLLEN retsz = 0;

    if ((!Phone) || (!RtuNo)) return false;
    memset(Phone, 0, sizeof(buflen));

    if (!connectDb(DBCONNSTRING)) return -1;

    //---Read in alarm_registry for regnumber
    memset(resphone, 0, sizeof(resphone));
    sprintf(sqlreq, "select rtuphone from rtu where rtumodel = 'SEA-ÑÝÀ' and rtusysno = %d ;", RtuNo);
    if (0 > osOdbcExecute(hdbc, &hstmt, (SQLCHAR*)sqlreq)) {
        disconnectDb();
        return -2;
    }
    SQLBindCol(hstmt, 1, SQL_C_CHAR, resphone, 50, &retsz);
    retcode = SQLFetch(hstmt);    // Should be the only record in alarm_registry
    osOdbcExecuteCancel(hstmt);
    if ((retcode == SQL_SUCCESS) || (retcode == SQL_SUCCESS_WITH_INFO)) {
        strncpy(Phone, resphone, buflen);
    } else {
        disconnectDb();
        return -3;
    }

    return 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CloseDB()
{
  disconnectDb();
  isdbconnected = false;
}

