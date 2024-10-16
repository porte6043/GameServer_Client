#include "Village3.h"
#include "LobbyProtocol.h"

CVillage3::CVillage3()
{}
void CVillage3::EnterRoom(DWORD64 SessionID, Param param)
{}
void CVillage3::LeaveRoom(DWORD64 SessionID)
{}
void CVillage3::SessionRelease(DWORD64 SessionID)
{}
void CVillage3::RecvToRoom(Param param)
{}
void CVillage3::Recv(DWORD64 SessionID, const en_PACEKT_LOBBY_TYPE& type, CPacket& packet)
{
	switch (type)
	{
	default:
		Disconnect(SessionID);
		break;
	}
}
void CVillage3::Update()
{}
