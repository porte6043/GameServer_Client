#ifndef __LOBBYSERVER_CHARACTER_H__
#define __LOBBYSERVER_CHARACTER_H__
#include <Windows.h>
#include <string>
using std::string;


#include "���� ���̺귯��/SerialRingBuffer.h"
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

	// ĳ���� ��ġ
	en_ROOM_NUMBER		roomNo;
	CBaseField*			pField;

	// ĳ���� �̸�
	string				Nickname;

	// �ɷ�ġ
	CStats				stats;

	// ����ǰ
	CInventory			inventory;

	// ģ�� ���
	vector<st_Friend>	Friends;
	vector<string>		AddFriends;

	// ĳ���� ����Ʈ
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
