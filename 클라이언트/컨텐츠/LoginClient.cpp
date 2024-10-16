#include "LoginClient.h"
#include "LoginProtocol.h"
#include "PacketInit.h"
using namespace ns_LoginClient;
#include "���� ���̺귯��/Monitor.h"

#include <iostream>
#include <vector>
#include <string>
using std::vector;
using std::cout;
using std::cin;
using std::endl;

LoginClient::LoginClient() : uid(0), done(false)
{
	// ȭ�� �ʱ�ȭ
	cs_Initial();
}

void LoginClient::OnConnectServer()
{
	login();

	return;
}

void LoginClient::OnDisconnectServer()
{
	return;
}

void LoginClient::OnRecv(CPacket& packet)
{
	DWORD type;
	packet >> type;
	switch (type)
	{
	case en_PACKET_SC_LOGIN_RES_LOGIN:
		Packet_Login(packet);
		break;
	case en_PACKET_SC_LOGIN_RES_SELECT_SERVER:
		Packet_Select_Server(packet);
		break;
	}

	return;
}

st_GameServerInfo LoginClient::getGameServerInfo()
{
	while (!done)
	{
		Sleep(1000);
	}

	wstring ip(serverInfo.ip.begin(), serverInfo.ip.end());

	return { uid, token, ip, serverInfo.port, serverInfo.serverName };
}


void LoginClient::login()
{
	string id;
	string password;

	cout << "id, password�� �Է��ϼ���" << endl;
	cout << "id : ";
	cin >> id;
	cout << "password : ";
	cin >> password;

	CPacket packet;
	Packet_Init_Login(packet, id, password);
	SendMSG(packet);

	cout << "�α��� ���Դϴ�...";
}


// packet 
void LoginClient::Packet_Login(CPacket& packet)
{
	BYTE status;
	DWORD64 serverCount;
	vector<ServerInfo> serverInfoes;
	packet >> status >> uid >> serverCount;
	for(int iCnt = 0; iCnt < serverCount; ++iCnt)
	{
		ServerInfo info;
		packet.GetData(info.ip, 16);
		packet >> info.port;
		packet.GetData(info.serverName, 20);
		packet >> info.serverStatus;

		serverInfoes.push_back(info);
	}

	
	switch (static_cast<en_STATUS_LOGIN>(status))
	{
	case en_STATUS_LOGIN::SUECCESS:
		cs_MoveCursor(0, 0);
		cs_ClearScreen();
		break;
	case en_STATUS_LOGIN::NO_EXIST_ID:
		cout << "���� ���� �ʴ� id�Դϴ�." << endl;
		login();
		return;
	case en_STATUS_LOGIN::NO_EXIST_PASSWORD:
		cout << "��й�ȣ�� ��ġ���� �ʽ��ϴ�." << endl;
		login();
		return;
	default:
		cout << "�� �� ���� �����Դϴ�." << endl;
		login();
		return;
	}

	
	// server ���� â
	int serverNo;
	for (int icnt = 0; icnt < serverInfoes.size(); ++icnt)
	{
		auto& info = serverInfoes[icnt];
		cout << icnt  << ". " << info.serverName.c_str() << " : " << info.getServerCongestion() << "(" << info.ip << ":" << info.port << ")" << endl;
	}
	while (1)
	{
		cout << "������ ���� ��ȣ�� �Է��ϼ��� : ";
		cin >> serverNo;

		if (serverNo < serverInfoes.size())
		{
			serverInfo = serverInfoes[serverNo];
			break;
		}
		else
		{
			cout << "�������� �ʴ� �����Դϴ�." << endl;
		}
	}

	CPacket Packet;
	Packet_Init_Select_Server(Packet, uid);
	SendMSG(Packet);

	return;
}

void LoginClient::Packet_Select_Server(CPacket& packet)
{
	packet.GetData(token, 64);
	Disconnect();
	done = true;
	return;
}