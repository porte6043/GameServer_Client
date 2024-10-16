#include "LoginServer.h"
#include "���� ���̺귯��/TextPasing.h"
#include "���� ���̺귯��/CCsvParser.h"
#include "CRedis.h"
#include "StringUtilities.h"
#include "PacketInit.h"
#include "Query.h"

LoginServer::LoginServer() : mysqlPool(1, 10)
{
	// ���� ����
	CTextPasing config;
	config.GetLoadData("Server.ini");
	CTextPasingCategory* ChatServer = config.FindCategory("LoginServer");
	ChatServer->GetValueChar(serverSet.IP_char, "IP");
	ChatServer->GetValueWChar(serverSet.IP_wchar, "IP");
	ChatServer->GetValueShort((short*)&serverSet.Port, "Port");
	ChatServer->GetValueShort((short*)&serverSet.WorkerThread, "WorkerThreads");
	ChatServer->GetValueShort((short*)&serverSet.ActiveThread, "ActiveThreads");
	ChatServer->GetValueBool(&serverSet.Nagle, "Nagle");
	ChatServer->GetValueBool(&serverSet.ZeroCopySend, "ZeroCopySend");
	ChatServer->GetValueShort((short*)&serverSet.SessionMax, "SessionMax");
	ChatServer->GetValueByte((char*)&serverSet.PacketCode, "PacketCode");
	ChatServer->GetValueByte((char*)&serverSet.PacketKey, "PacketKey");
	ChatServer->GetValueByte((char*)&LogLevel, "LogLevel");

	LOG_LEVEL(LogLevel);

	// �ܼ� �ʱ�ȭ
	cs_Initial();

	// mysql �ʱ�ȭ
	/*
	mysql_init ���� Ư�� ������ �� mysql_library_init�� ȣ���ϴµ� mysql_library_init�� thread safe �����ʱ� ������ mysql_init�� thread safe���� �ʴ�
	���� mysql_init�� ��Ƽ���� ����� ��� mutex ���� ����ְų� �̸� mysql_library_init�� ȣ���� �ָ� thread safe �ϴ�
	*/
	if (mysql_library_init(0, NULL, NULL))
	{
		LOG(L"server_LoginServer", CLog::LEVEL_ERROR, L"mysql init ����");
		Crash();
	}
}
LoginServer::~LoginServer()
{
	mysql_library_end();
}


bool LoginServer::OnAcceptRequest(WCHAR* IP, WORD Port)
{
	return true;
}
void LoginServer::OnSessionConnected(DWORD64 SessionID)
{
	cout << "���� �Ͽ����ϴ�." << endl;
	return;
}
void LoginServer::OnSessionRelease(DWORD64 SessionID)
{
	cout << "���� ���� �Ͽ����ϴ�." << endl;
	return;
}
void LoginServer::OnRecv(DWORD64 SessionID, CPacket& packet)
{
	int type;
	packet >> type;

	switch (type)
	{
	case en_PACKET_CS_LOGIN_REQ_LOGIN:
		Packet_Login(SessionID, packet);
		break;

	case en_PACKET_CS_LOGIN_REQ_SELECT_SERVER:
		Packet_SelectServer(SessionID, packet);
		break;

	default:
		LOG(L"server_LoginServer", CLog::LEVEL_DEBUG, L"���� ���� ���� ��Ŷ");
		Disconnect(SessionID);
		break;
	}


	return;
}


// packet
void LoginServer::Packet_Login(DWORD64 SessionID, CPacket& packet)
{
	char id[20];
	char password[20];
	packet.GetData(id, 20);
	packet.GetData(password, 20);
	if (packet.GetUseSize() != 0)
	{
		Disconnect(SessionID);
		return;
	}

	// DB���� id, password Ȯ���ϱ�
	mysql* conn = mysqlPool.Alloc();
	if (mysql_query(&conn->conn, Query_Select_Account(id).c_str())) 	// mysql_query == 0 ����, mysql_errno == 0 ���� ����
	{
		LOG(L"server_LoginServer", CLog::LEVEL_ERROR, L"mysql_query ���� error:%d", mysql_errno(&conn->conn));
		Disconnect(SessionID);
		return;
	}
	MYSQL_RES* sql_res = mysql_store_result(&conn->conn);
	if (sql_res->row_count == 0)
	{
		// ���� ���� �ʴ� ���̵� �Դϴ�.
		CPacket packet;
		Packet_Init_Login(packet, static_cast<BYTE>(en_STATUS_LOGIN::NO_EXIST_ID), 0, {});
		SendMSG(SessionID, packet);
		mysql_free_result(sql_res);
		return;
		
	}

	MYSQL_ROW sql_row = mysql_fetch_row(sql_res);
	string idByDB = sql_row[0];
	string passwordByDB = sql_row[1];
	DWORD64 uid = stoull(sql_row[2]);
	mysql_free_result(sql_res);

	if (passwordByDB.compare(password) != 0)
	{
		// ��й�ȣ�� �ٸ��ϴ�.
		CPacket packet;
		Packet_Init_Login(packet, static_cast<BYTE>(en_STATUS_LOGIN::NO_EXIST_PASSWORD), 0, {});
		SendMSG(SessionID, packet);
		return;
	}
	
	// redis���� ���� ���� �۽�
	CRedis redis;
	auto opt = redis.hgetall("ServerStatus");
	if (!opt.has_value())
	{
		Disconnect(SessionID);
		return;
	}
	auto& serverStatus = opt.value();
	CPacket Packet;
	Packet_Init_Login(Packet, static_cast<BYTE>(en_STATUS_LOGIN::SUECCESS), uid, serverStatus);
	SendMSG(SessionID, Packet);
	

	mysqlPool.Free(conn);
	return;
}

void LoginServer::Packet_SelectServer(DWORD64 SessionID, CPacket& packet)
{
	DWORD64 uid;
	packet >> uid;
	if (packet.GetUseSize() != 0)
	{
		Disconnect(SessionID);
		return;
	}

	// ��ū ����
	string token = "qwertyyuuihdergfdertyhgfde56kjhgfeeuijhgfdwertyyhgfdswerttfdrrr";
	
	// ��ū ���
	CRedis redis;
	while (!redis.setex(to_string(uid), 10, token)) {}

	// ��ū ���� �� ���� ����
	CPacket Packet;
	Packet_Init_Select_Server(Packet, token);
	SendMSG_Disconnect(SessionID, Packet, 10);

	return;
}