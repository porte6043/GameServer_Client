#include "Village1.h"
#include "LobbyProtocol.h"

CVillage1::CVillage1()
{}
void CVillage1::EnterRoom(DWORD64 SessionID, Param param)
{}
void CVillage1::LeaveRoom(DWORD64 SessionID)
{}
void CVillage1::SessionRelease(DWORD64 SessionID)
{}
void CVillage1::RecvToRoom(Param param)
{}
void CVillage1::Recv(DWORD64 SessionID, const en_PACEKT_LOBBY_TYPE& type, CPacket& packet)
{
	switch (type)
	{
	default:
		Disconnect(SessionID);
		break;
	}
}
void CVillage1::Update()
{}
