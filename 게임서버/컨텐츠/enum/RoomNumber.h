#ifndef __LOBBYSERVER_ROOMNUMBER_H__
#define __LOBBYSERVER_ROOMNUMBER_H__
#include <windows.h>
#include <string>
using std::string;

enum class en_ROOM_NUMBER : unsigned int
{
	Lobby = 0,

	// ´ë·ú 1 : 1~99
	Village1 = 1,
	Village2,
	Village3,

	// ´ë·ú 2 : 100~199
	MonsterField1 = 100,
	MonsterField2,
	MonsterField3,


	// ÀÎ´ø¿ë 10000~19999
	Dungeon = 10000
};


string getFieldName(DWORD RoomNo);


#endif // __LOBBYSERVER_ROOMNUMBER_H__
