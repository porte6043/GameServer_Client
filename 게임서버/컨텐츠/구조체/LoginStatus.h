#ifndef __LOBBYSERVER_LOGINSTATUS_H__
#define __LOBBYSERVER_LOGINSTATUS_H__
#include <string>
#include <Windows.h>
#include "Aliases.h"

struct st_LoginStatus
{
	string	IP;
	WORD	Port;
	Sid		SessionID;

	st_LoginStatus() {}
	st_LoginStatus(string ip, WORD port, Sid sessionID) : IP(ip), Port(port), SessionID(sessionID) {}
};

#endif