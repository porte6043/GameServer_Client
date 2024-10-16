#pragma once
#include <Windows.h>
#include "CBaseField.h"

class CLobbyServer;
enum en_PACEKT_LOBBY_TYPE : short;

class CVillage1: public CBaseField
{
public:
	CVillage1();

private:
	void EnterRoom(DWORD64 SessionID, Param param) override;
	void LeaveRoom(DWORD64 SessionID) override;
	void SessionRelease(DWORD64 SessionID) override;
	void RecvToRoom(Param param) override;
	void Recv(DWORD64 SessionID, const en_PACEKT_LOBBY_TYPE& type, CPacket& packet) override;
	void Update() override;
};