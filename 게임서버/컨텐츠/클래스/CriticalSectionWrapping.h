#ifndef __LOBBYSERVER_CRITICALSECTIONWRAPPING_H__
#define __LOBBYSERVER_CRITICALSECTIONWRAPPING_H__
#include <Windows.h>

class lock_cs
{
private:
	CRITICAL_SECTION* cs;
public:
	lock_cs() = delete;
	lock_cs(CRITICAL_SECTION* CS)
	{
		cs = CS;
		EnterCriticalSection(cs);
	}
	~lock_cs()
	{
		LeaveCriticalSection(cs);
	}
};

#endif