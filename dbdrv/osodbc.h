#ifndef _ODBCFNCH
#define  _ODBCFNCH
#include <sqlext.h>

int osOdbcInit( SQLHANDLE* henv, int version=3 );
int osOdbcFree( SQLHANDLE henv );
int osOdbcConnect( SQLHANDLE henv, SQLCHAR* ds_name, SQLHANDLE* hdbc, SQLCHAR* user=NULL, SQLCHAR* pswd=NULL, int connecttout=30 );
int osOdbcDisconnect( SQLHANDLE hdbc );
int osOdbcExecute( SQLHANDLE hdbc, SQLHANDLE* hstmt,SQLCHAR* statementtext, bool doalloc=true );
int osOdbcExecuteCancel( SQLHANDLE hstmt );

#endif
