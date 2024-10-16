#include "Character.h"
#include "네트워크/NetworkDefine.h"
#include "CBaseField.h"

st_User::st_User()
{
	SessionID = INVALID_SESSIONID;
	uid = 0;
	LastTimeTick = 0;
	roomNo = static_cast<en_ROOM_NUMBER>(INVALID_ROOMNO);
	pField = nullptr;
}

CBaseField* st_User::getField()
{
	return pField;
}

void st_User::setField(const en_ROOM_NUMBER& RoomNo, CBaseField* pField)
{
	this->roomNo = RoomNo;
	this->pField = pField;
}

void st_User::clear()
{
	SessionID = INVALID_SESSIONID;
	uid = 0;
	LastTimeTick = 0;
	roomNo = static_cast<en_ROOM_NUMBER>(INVALID_ROOMNO);
	pField = nullptr;
	Nickname.clear();
	stats.clear();
	inventory.clear();
	Friends.clear();
	AddFriends.clear();
	Characters.clear();
}

void st_User::sendMassage(CPacket& packet)
{
	pField->SendMSG(SessionID, packet);
}

void st_User::disconnect()
{
	pField->Disconnect(SessionID);
}