#include <windows.h>
#include <sqlext.h>
#pragma hdrstop

/*
 * vpr,170610
 *
 *      Lidrary of simple functions to handle DBs via ODBC
 *
 *      OS:     Windows,
 *              nix (UNIX ODBC), not tested yet
 */


/*******************************************************************************
 *
 *      ODBC init
 *
 * Remarks:
 *      Initializes the ODBC for application by allocating the environment handle
 *      Sets the ODBC driver version
 * Parameters:
 *      henv    - OUT, pointer to memory location for odbc environment handle
 *      version - IN, ODBC driver version: 2,3,..., default is 3
 * Returns:
 *              0 on success,
 *              -1 if bad parameter was passed
 *              -2 if handle allocation error occurred
 *              -3 if ODBC versioning error occurred
 *
 */
int osOdbcInit( SQLHANDLE* henv, int odbcversion )
{
SQLRETURN retcode;
void* ver;

  if( !henv )return -1;
  retcode = SQLAllocHandle( SQL_HANDLE_ENV, SQL_NULL_HANDLE, henv );
  if ( ( retcode != SQL_SUCCESS ) && ( retcode != SQL_SUCCESS_WITH_INFO ) ) return -2;

  switch( odbcversion ){
        case 2:         ver = (void*)SQL_OV_ODBC2; break;
        default:        ver = (void*)SQL_OV_ODBC3; break;
  }
  retcode = SQLSetEnvAttr( *henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)ver, 0 );
  if ( ( retcode != SQL_SUCCESS ) && ( retcode != SQL_SUCCESS_WITH_INFO ) ) {
        SQLFreeHandle( SQL_HANDLE_ENV, *henv );
        return -3;
  }
  return 0;
}
/*******************************************************************************
 *
 *      ODBC free
 *
 * Remarks:
 *      Frees the ODBC resources allocated for the app
 * Parameters:
 *      henv    - IN, the odbc environment handle
 * Returns:
 *              0 on success,
 *              -1 if bad parameter
 *              -2 if handle operation error occurred
 */
int osOdbcFree( SQLHANDLE henv )
{
SQLRETURN retcode;

        retcode = SQLFreeHandle( SQL_HANDLE_ENV, henv );
        if( SQL_INVALID_HANDLE == retcode )     return -1;
        if( SQL_ERROR == retcode )              return -2;
        return 0;
}
/*******************************************************************************
 *
 *      ODBC connect
 * Remarks:
 *      Establishes connection to DB
 * Parameters:
 *      henv    - IN, the odbc environment handle
 *      ds_name - IN, pointer to ASCII string with data source name
 *      hdbc    - OUT, the pointer to ODBC connection handle
 *      user    - IN, pointer to C-string with username       (default is NULL)
 *      pswd    - IN, pointer to C-string with user password  (default is NULL)
 *      connecttout - IN, operation timeout in seconds        (default is 30sec)
 * Returns:
 *              0 on success,
 *              -1 if bad parameter was passed
 *              -2 if handle allocation error occurred
 *              -4 if in connect error occurred
 */
int osOdbcConnect( SQLHANDLE henv, SQLCHAR* ds_name, SQLHANDLE* hdbc, SQLCHAR* user, SQLCHAR* pswd, int connecttout )
{
SQLRETURN retcode;

  if( ( !ds_name ) || ( !hdbc ) )return -1;

  // Allocate connection handle
  retcode = SQLAllocHandle( SQL_HANDLE_DBC, henv, hdbc );
  if( SQL_INVALID_HANDLE == retcode )                                           return -1;
  if ( ( retcode != SQL_SUCCESS ) && ( retcode != SQL_SUCCESS_WITH_INFO ) )     return -2;

  // Set login timeout, skip for lazy error testing here, it'll rise on next step
  retcode = SQLSetConnectAttr( *hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)connecttout, 0 );

  // Connect to data source
  retcode = SQLConnect( *hdbc,  ds_name, SQL_NTS,  user, SQL_NTS, pswd, SQL_NTS );
  if ( ( retcode != SQL_SUCCESS ) && ( retcode != SQL_SUCCESS_WITH_INFO ) ) {
        SQLFreeHandle( SQL_HANDLE_ENV, *hdbc );
        return -4;
  }
  return 0;
}
/*******************************************************************************
 *
 *      ODBC disconnect
 * Remarks:
 *      Closes connection to DB
 * Parameters:
 *      hdbc    - IN, the ODBC connection handle
 * Returns:
 *              0 on success,
 *              -1 if bad parameter was passed
 *              -2 if handle-operation error occurred
 *              -3 if in disconnect error occurred
 */
int osOdbcDisconnect( SQLHANDLE hdbc )
{
SQLRETURN retcode;

        retcode = SQLDisconnect( hdbc );
        if( SQL_INVALID_HANDLE == retcode )     return -1;
        if ( ( retcode != SQL_SUCCESS ) && ( retcode != SQL_SUCCESS_WITH_INFO ) ) return -3;

        retcode = SQLFreeHandle( SQL_HANDLE_ENV, hdbc );
        if ( ( retcode != SQL_SUCCESS ) && ( retcode != SQL_SUCCESS_WITH_INFO ) ) return -2;

return 0;
}
/*******************************************************************************
 *
 *      ODBC execute
 * Remarks:
 *      Executes the   select/update/insert/procedure statements
 *              Here is an example how to invoke a stored procedure:
 *        SQLExecDirect( hstmt, (UCHAR*)"{? = call TestParm(?)}", SQL_NTS );
 * Parameters:
 *      hdbc    - IN, the ODBC connection handle
 *      hstmt   - IN/OUT, the pointer to memory location for ODBC statement handle
 *      statementtext   - IN, the pointer to C-string with statement for execute
 *      doalloc - IN,  if true(default), function itself allocates the statement handle,
 *                      otherwise uses the handle, pointed by the hsmt parameter
 * Returns:
 *              0 on success,
 *              -1 if bad parameter was passed
 *              -2 if handle-operation error occurred
 *              -3 if in execute error occurred
 */
int osOdbcExecute( SQLHANDLE hdbc, SQLHANDLE* hstmt,SQLCHAR* statementtext, bool doalloc )
{
SQLRETURN retcode;
SQLCHAR SQLState[10], MessageText[100];
SQLINTEGER NativeError;
SQLSMALLINT   TextLengthPtr;

  if( ( !hstmt ) || ( !statementtext ) )        return -1;
  if(doalloc){
  retcode = SQLAllocHandle( SQL_HANDLE_STMT, hdbc, hstmt );
          if( SQL_INVALID_HANDLE == retcode )           return -1;
          if ( ( retcode != SQL_SUCCESS ) && ( retcode != SQL_SUCCESS_WITH_INFO ) ) return -2;
  }

  retcode = SQLExecDirect( *hstmt,  statementtext, SQL_NTS );
  if ( ( retcode != SQL_SUCCESS ) && ( retcode != SQL_SUCCESS_WITH_INFO ) ) {
        SQLGetDiagRec (SQL_HANDLE_STMT,*hstmt,1,SQLState,&NativeError,MessageText,sizeof(MessageText),&TextLengthPtr);
        return -3;
  }

  return 0;
}
/*******************************************************************************
 *
 *      ODBC cancel execute
 * Remarks:
 *      Cancels the execution of given statement & all linked asynk ops
 * Parameters:
 *      hstmt   - OUT, the ODBC statement handle
 * Returns:
 *              0 on success,
 *              -1 if bad parameter was passed
 *              -2 if handle-operation error occurred
 */
int osOdbcExecuteCancel( SQLHANDLE hstmt )
{
SQLRETURN retcode;

        retcode = SQLCancel( hstmt); // for termination async ops if exist
        if( SQL_INVALID_HANDLE == retcode )           return -1;

        retcode = SQLFreeHandle( SQL_HANDLE_STMT, hstmt );
        if( SQL_INVALID_HANDLE == retcode )           return -1;
        if ( ( retcode != SQL_SUCCESS ) && ( retcode != SQL_SUCCESS_WITH_INFO ) ) return -2;
return 0;
}
