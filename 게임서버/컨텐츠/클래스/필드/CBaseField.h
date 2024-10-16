#ifndef __LOBBYSERVER_CBASEFIELD_H__
#define __LOBBYSERVER_CBASEFIELD_H__
#include <unordered_map>

#include "네트워크/CRoom2.h"

// 정의 및 프로토콜
#include "Aliases.h"

// DB
#include "DB.h"

class st_User;
class CLobbyServer; 
enum en_PACEKT_LOBBY_TYPE : short;


class CBaseField : public CRoom2
{
protected:
	std::unordered_map<Sid, st_User*> UserMap;

public:
	CLobbyServer* Server;
	DB DB_Write;
	DB DB_Read; 

	string fieldName;


private:	
	void OnSetNetwork(CWanServerRoom2* network) override;
	void OnRecv(DWORD64 SessionID, CPacket& packet) override;
	void OnEnterRoom(DWORD64 SessionID, Param param) override;
	void OnLeaveRoom(DWORD64 SessionID) override;
	void OnSessionKickOut(DWORD64 SessionID) override;
	void OnSessionRelease(DWORD64 SessionID) override;
	void OnRecvToRoom(Param param) override;
	void OnUpdate() override;

public:
	CBaseField();

	virtual void EnterRoom(DWORD64 SessionID, Param param) = 0;
	virtual void LeaveRoom(DWORD64 SessionID) = 0;
	virtual void SessionRelease(DWORD64 SessionID) = 0;
	virtual void RecvToRoom(Param param) = 0;
	virtual void Recv(DWORD64 SessionID, const en_PACEKT_LOBBY_TYPE& type, CPacket& packet) = 0;
	virtual void Update() = 0; 

	bool sendToRoom(const Sid& sessionID, Param param);

	bool MoveRoom(const Sid& SessionID, const DWORD& RoomNo, st_User* user);


//	packet 함수
protected:
	void Packet_Move_Field(const Sid& SessionID, CPacket& packet);
	void Packet_Ready_Field(const Sid& SessionID, CPacket& packet);
	void Packet_Move_Channel(const Sid& SessionID, CPacket& packet);
	void Packet_Channel_List(const Sid& SessionID, CPacket& packet);

	void Packet_Store_List(const Sid& SessionID, CPacket& packet);
	void Packet_Buy_Store_Item(const Sid& SessionID, CPacket& packet);
	void Packet_Show_Me_The_Money(const Sid& SessionID, CPacket& packet);
	void Packet_Use_Item(const Sid& SessionID, CPacket& packet);


	void Packet_Add_Friend(const Sid& SessionID, CPacket& packet);
	void Packet_Accept_Add_Friend(const Sid& SessionID, CPacket& packet);
	void Packet_Friend_List(const Sid& SessionID, CPacket& packet);
	void Packet_Send_Whishper(const Sid& SessionID, CPacket& packet);
};

#endif