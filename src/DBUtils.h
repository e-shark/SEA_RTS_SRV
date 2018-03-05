#ifndef DBUtilsH
#define DBUtilsH

extern char DBCONNSTRING[256];

int GetPhoneForRTU(int RtuNo, char* Phone, int buflen);
int TestDb();
void CloseDB();




#endif
