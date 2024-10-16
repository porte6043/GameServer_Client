#include "Village2.h"
#include "LobbyProtocol.h"

CVillage2::CVillage2()
{}
void CVillage2::EnterRoom(DWORD64 SessionID, Param param)
{}
void CVillage2::LeaveRoom(DWORD64 SessionID)
{}
void CVillage2::SessionRelease(DWORD64 SessionID)
{}
void CVillage2::RecvToRoom(Param param)
{}
void CVillage2::Recv(DWORD64 SessionID, const en_PACEKT_LOBBY_TYPE& type, CPacket& packet)
{
	switch (type)
	{
	default:
		Disconnect(SessionID);
		break;
	}
}
void CVillage2::Update()
{}
