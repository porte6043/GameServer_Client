#ifndef __SERVERSTATUSTYPE_H__
#define __SERVERSTATUSTYPE_H__
#include <Windows.h>
#include <string>
using std::string;

enum class en_SERVER_STATUS_TYPE
{
	GREEN = 1,
	ORANGE,
	RED
};

string getServerStatus(BYTE status);

#endif