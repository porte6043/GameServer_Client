#include "LoginClient.h"
#include "LoginProtocol.h"
#include "PacketInit.h"
using namespace ns_LoginClient;
#include "공용 라이브러리/Monitor.h"

#include <iostream>
#include <vector>
#include <string>
using std::vector;
using std::cout;
using std::cin;
using std::endl;

LoginClient::LoginClient() : uid(0), done(false)
{
	// 화면 초기화
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

	cout << "id, password를 입력하세요" << endl;
	cout << "id : ";
	cin >> id;
	cout << "password : ";
	cin >> password;

	CPacket packet;
	Packet_Init_Login(packet, id, password);
	SendMSG(packet);

	cout << "로그인 중입니다...";
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
		cout << "존재 하지 않는 id입니다." << endl;
		login();
		return;
	case en_STATUS_LOGIN::NO_EXIST_PASSWORD:
		cout << "비밀번호가 일치하지 않습니다." << endl;
		login();
		return;
	default:
		cout << "알 수 없는 오류입니다." << endl;
		login();
		return;
	}

	
	// server 선택 창
	int serverNo;
	for (int icnt = 0; icnt < serverInfoes.size(); ++icnt)
	{
		auto& info = serverInfoes[icnt];
		cout << icnt  << ". " << info.serverName.c_str() << " : " << info.getServerCongestion() << "(" << info.ip << ":" << info.port << ")" << endl;
	}
	while (1)
	{
		cout << "입장할 서버 번호를 입력하세요 : ";
		cin >> serverNo;

		if (serverNo < serverInfoes.size())
		{
			serverInfo = serverInfoes[serverNo];
			break;
		}
		else
		{
			cout << "존재하지 않는 서버입니다." << endl;
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