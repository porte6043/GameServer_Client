#include "RoomManager.h"


// enum 값을 문자열로 매핑하기 위한 맵
const map<en_ROOM_NUMBER, string> RoomMap =
    {
    {en_ROOM_NUMBER::village1, "village1"},
    {en_ROOM_NUMBER::village2, "village2"},
    {en_ROOM_NUMBER::village3, "village3"}
    };






string RoomManager::getRoomName(DWORD roomNo)
{
    auto iter = RoomMap.find(static_cast<en_ROOM_NUMBER>(roomNo));
    if (iter == RoomMap.end())
        return "";
    else
        return iter->second;
}

const map<en_ROOM_NUMBER, string>& RoomManager::getRoomMap()
{
    return RoomMap;
}
