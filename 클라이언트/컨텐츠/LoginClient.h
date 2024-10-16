#ifndef __LOGINCLIENT_H__
#define __LOGINCLIENT_H__
#include "��Ʈ��ũ/CWanClient.h"
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
	// ������ ������� ��
	void OnConnectServer() override;

	// ������ ������ �������� ��
	void OnDisconnectServer() override;

	// Message ���� �Ϸ� �� Contents Packet ó���� �մϴ�.
	void OnRecv(CPacket& packet) override;

public:
	// �α��� ������ �����Ͽ� ���� ������ �����ϱ� ���� ������ �����ɴϴ�.
	st_GameServerInfo getGameServerInfo();

private:
	void login();

	// packet 
	void Packet_Login(CPacket& packet);
	void Packet_Select_Server(CPacket& packet);
};


#endif