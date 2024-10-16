#ifndef __LOBBYSERVER_LOBBY_H__
#define __LOBBYSERVER_LOBBY_H__
#include <unordered_map>
#include <list>


// 라이브러리
#include "네트워크/CRoom2.h"
#include "Aliases.h"

// DB
#include "DB.h"


class CLobbyServer;
class st_User;

class CLobby : public CRoom2
{
private:
	CLobbyServer* Server;
	std::unordered_map<Sid, st_User*> UserMap;

	std::list<std::pair<Sid, INT64>> DuplicationUser;	// SessionID, TimeOut

	// DB
	DB	db;

private:
	virtual void OnSetNetwork(CWanServerRoom2* network)  override;
	virtual void OnRecv(DWORD64 SessionID, CPacket& packet) override;
	virtual void OnEnterRoom(DWORD64 SessionID, Param param) override;
	virtual void OnLeaveRoom(DWORD64 SessionID) override;
	virtual void OnSessionKickOut(DWORD64 SessionID) override;
	virtual void OnSessionRelease(DWORD64 SessionID) override;
	virtual void OnRecvToRoom(Param param) override;
	virtual void OnUpdate() override;

public:		
	CLobby();
	~CLobby();

private:
	st_User* FindUser(const Sid& SessionID);
	

};

#endif // __LOBBYSERVER_LOBBY_H__
