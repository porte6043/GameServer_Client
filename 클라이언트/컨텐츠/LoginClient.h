#ifndef __LOGINCLIENT_H__
#define __LOGINCLIENT_H__
#include "네트워크/CWanClient.h"
#include "GameServerInfo.h"
#include "ServerStatusType.h"



class LoginClient : public CWanClient
{
	struct ServerInfo
	{
		string	ip;
		WORD	port;
		string	serverName;
		BYTE	serverStatus;

		string getServerCongestion()
		{
			switch (static_cast<en_SERVER_STATUS_TYPE>(serverStatus))
			{
			case en_SERVER_STATUS_TYPE::GREEN:
				return "Green";
			case en_SERVER_STATUS_TYPE::ORANGE:
				return "Orange";
			case en_SERVER_STATUS_TYPE::RED:
				return "Red";
			default:
				return "";
			}
		}
	};

	DWORD64 uid;
	string token;
	ServerInfo serverInfo;
	bool done;

public:
	LoginClient();

protected:
	// 서버와 연결됐을 때
	void OnConnectServer() override;

	// 서버와 연결이 끊어졌을 때
	void OnDisconnectServer() override;

	// Message 수신 완료 후 Contents Packet 처리를 합니다.
	void OnRecv(CPacket& packet) override;

public:
	// 로그인 서버에 접속하여 게임 서버에 접속하기 위한 정보를 가져옵니다.
	st_GameServerInfo getGameServerInfo();

private:
	void login();

	// packet 
	void Packet_Login(CPacket& packet);
	void Packet_Select_Server(CPacket& packet);
};


#endif