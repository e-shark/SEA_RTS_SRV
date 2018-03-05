
#include "utils.h"

double pow10(int x) { return pow(10, x); };

std::string IntToStr(int Value) 
{ 
	std::string res; 
	char s[50]; 
	sprintf(s, "%d", Value); 
	res = s; 
	return res;
};

std::string IntToHex(int Value, int Digits) 
{
	std::string res;
	int d = Digits;
	char fs[10];
	char s[256];
	if (d > 255) d = 255;
	sprintf(fs, " .%dx", d);
	fs[0] = '%';
	sprintf(s, fs, Value);
	res = s;
	return res;
};
