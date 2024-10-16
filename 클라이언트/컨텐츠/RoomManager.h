#pragma once

#include <Windows.h>
#include <string>
#include <map>
using std::map;
using std::string;

enum class en_ROOM_NUMBER : unsigned int
{
	Lobby = 0,

	// ´ë·ú 1 : 1~99

	village1 = 1,
	village2,
	village3,


	// ´ë·ú 2 : 100~199


	// ÀÎ´ø¿ë 10000~19999
	Dungeon = 10000
};


class RoomManager
{
public:
	string getRoomName(DWORD roomNo);

	const map<en_ROOM_NUMBER, string>& getRoomMap();
};