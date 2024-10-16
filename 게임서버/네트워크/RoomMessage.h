#ifndef __NETWORK_ROOMMESSAGE_H__
#define __NETWORK_ROOMMESSAGE_H__
#include <Windows.h>
#include <네트워크/NetworkDefine.h>

union Param
{
	struct FIELD
	{
		LONG64 ptr : 47;
		LONG64 idx : 17;
	}field;
	LONG64 value;
	Param() : value(0) {}
	Param(const LONG64&& v) : value(v) {}
	Param(const void* v) : value(reinterpret_cast<LONG64>(v)) {}
};

typedef struct Job_RoomMessageQ
{
	en_JOB_ROOM Type;
	DWORD64		SessionID;
	Param		param;
};


#endif