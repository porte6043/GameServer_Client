#pragma once
#include <Windows.h>
#include <string>

struct st_GameServerInfo
{
	DWORD64			uid;
	std::string		token;
	std::wstring	ip;
	WORD			port;
	std::string		serverName;
};