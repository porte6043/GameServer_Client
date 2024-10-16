#ifndef __LOBBYSERVER_SERVERSTATUS_H__
#define __LOBBYSERVER_SERVERSTATUS_H__
#include <Windows.h>
#include <string>
using std::string;

struct st_ServerStatus
{	
	string		ServerName;
	string		IP;
	WORD		Port;
	BYTE		StatusType;	// 채널 인원 상황
	st_ServerStatus(string ip, WORD port, string serverName, BYTE statusType) : ServerName(serverName), IP(ip), Port(port), StatusType(statusType) {}
};

#endif