#ifndef __LOBBYSERVER_CHARACTER_H__
#define __LOBBYSERVER_CHARACTER_H__
#include <Windows.h>
#include <string>
using std::string;


#include "공용 라이브러리/SerialRingBuffer.h"
#include "Aliases.h"
#include "Inventory.h"
#include "CharacterStats.h"
#include "Friend.h"
#include "CharacterSeq.h"
#include "RoomNumber.h"

class CBaseField;

class st_User
{
public:
	Sid					SessionID;
	UID					uid;
	DWORD64				LastTimeTick;	// heartbeat

	// 캐릭터 위치
	en_ROOM_NUMBER		roomNo;
	CBaseField*			pField;

	// 캐릭터 이름
	string				Nickname;

	// 능력치
	CStats				stats;

	// 소지품
	CInventory			inventory;

	// 친구 목록
	vector<st_Friend>	Friends;
	vector<string>		AddFriends;

	// 캐릭터 리스트
	vector<st_CharacterSeq>	Characters;


public:
	st_User();

	CBaseField* getField();

	void setField(const en_ROOM_NUMBER& RoomNo, CBaseField* pField);

	void clear();

	void sendMassage(CPacket& packet);

	void disconnect();

};

#endif // __LOBBYSERVER_CHARACTER_H__
