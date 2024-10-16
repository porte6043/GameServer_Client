#include "RoomNumber.h"


string getFieldName(DWORD RoomNo)
{
	switch (static_cast<en_ROOM_NUMBER>(RoomNo))
	{
	case en_ROOM_NUMBER::Lobby:
		return "Lobby";
	case en_ROOM_NUMBER::Village1:
		return "Village1";
	case en_ROOM_NUMBER::Village2:
		return "Village2";
	case en_ROOM_NUMBER::Village3:
		return "Village3";
	case en_ROOM_NUMBER::MonsterField1:
		return "MonsterField11";
	case en_ROOM_NUMBER::MonsterField2:
		return "MonsterField12";
	case en_ROOM_NUMBER::MonsterField3:
		return "MonsterField113";
	case en_ROOM_NUMBER::Dungeon:
		return "InstanceDungeon";
	}
}