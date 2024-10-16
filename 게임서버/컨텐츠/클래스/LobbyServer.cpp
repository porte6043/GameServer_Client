#include "LobbyServer.h"

// 전역 함수
#include "CriticalSectionWrapping.h"
#include "PacketInit.h"
#include "StringUtilities.h"

// 필드
#include "Lobby.h"
#include "Village1.h"
#include "Village2.h"
#include "Village3.h"

// 구조체
#include "Character.h"
#include "RedisTask.h"
#include "ServerStatus.h"
#include "Friend.h"
#include "Money.h"

// 정의 및 프로토콜
#include "LobbyProtocol.h"
#include "RoomNumber.h"
#include "RoomTaskType.h"
#include "RedisTaskType.h"
#include "ServerStatusType.h"

// DB
#include "CRedis.h"
#include "DB.h"

// 매니저
#include "StoreManager.h"
#include "ItemManager.h"

CLobbyServer::CLobbyServer()
{
	// 서버 설정
	CTextPasing config;
	config.GetLoadData("Server.ini");
	CTextPasingCategory* ChatServer = config.FindCategory("LobbyServer");
	ChatServer->GetValueChar(ServerName, "ServerName");
	ChatServer->GetValueChar(ServerSet.IP_char, "IP");
	ChatServer->GetValueWChar(ServerSet.IP_wchar, "IP");
	ChatServer->GetValueShort((short*)&ServerSet.Port, "Port");
	ChatServer->GetValueShort((short*)&ServerSet.WorkerThread, "WorkerThreads");
	ChatServer->GetValueShort((short*)&ServerSet.ActiveThread, "ActiveThreads");
	ChatServer->GetValueBool(&ServerSet.Nagle, "Nagle");
	ChatServer->GetValueBool(&ServerSet.ZeroCopySend, "ZeroCopySend");
	ChatServer->GetValueShort((short*)&ServerSet.SessionMax, "SessionMax");
	ChatServer->GetValueByte((char*)&ServerSet.PacketCode, "PacketCode");
	ChatServer->GetValueByte((char*)&ServerSet.PacketKey, "PacketKey");
	ChatServer->GetValueByte((char*)&LogLevel, "LogLevel");

	LOG_LEVEL(LogLevel);

	// 매니저 데이터 초기화
	CItemManager	itemManager;	itemManager.setItemList();
	CStoreManager	storeManager;	storeManager.setStoreList();
	


	// 초기화
	UserPool.TlsPoolInit(100, 10);


	// redis task key 생성 ( key = IP:Port:TaskType )
	RedisTaskKeys.push_back(
		{
		en_REDIS_TASK_TYPE::WHISPER,
		CombineStrings({
			ServerSet.IP_char,
			to_string(ServerSet.Port),
			to_string(static_cast<int>(en_REDIS_TASK_TYPE::WHISPER))},
			':')
		}
	);
	RedisTaskKeys.push_back(
		{
		en_REDIS_TASK_TYPE::ADD_FRIEND,
		CombineStrings({
			ServerSet.IP_char,
			to_string(ServerSet.Port),
			to_string(static_cast<int>(en_REDIS_TASK_TYPE::ADD_FRIEND))},
			':')
		}
	);
	RedisTaskKeys.push_back(
		{
		en_REDIS_TASK_TYPE::ACCEPT_ADD_FRIEND,
		CombineStrings({
			ServerSet.IP_char,
			to_string(ServerSet.Port),
			to_string(static_cast<int>(en_REDIS_TASK_TYPE::ACCEPT_ADD_FRIEND))},
			':')
		}
	);
	RedisTaskKeys.push_back(
		{
		en_REDIS_TASK_TYPE::DUPLICATION_LOGIN,
		CombineStrings({
			ServerSet.IP_char,
			to_string(ServerSet.Port),
			to_string(static_cast<int>(en_REDIS_TASK_TYPE::DUPLICATION_LOGIN))},
			':')
		}
	);

	// 필드 생성
	CreateRoom<CLobby>(static_cast<DWORD>(en_ROOM_NUMBER::Lobby));
	CreateRoom<CVillage1>(static_cast<DWORD>(en_ROOM_NUMBER::Village1));
	CreateRoom<CVillage2>(static_cast<DWORD>(en_ROOM_NUMBER::Village2));
	CreateRoom<CVillage3>(static_cast<DWORD>(en_ROOM_NUMBER::Village3));
	SetBasicRoom(static_cast<DWORD>(en_ROOM_NUMBER::Lobby));

	// Redis 타이머 생성
	auto bindfunction1 = std::bind(&CLobbyServer::Redis_Task_Execute, this);
	auto bindfunction2 = std::bind(&CLobbyServer::Redis_UpdateServerStatus, this);
	RegisterTimer(bindfunction2, 1000);
	RegisterTimer(bindfunction1, 1000);
}

CLobbyServer::~CLobbyServer()
{
}

bool CLobbyServer::OnAcceptRequest(WCHAR* IP, WORD Port)
{
	return true;
}


st_User* CLobbyServer::allocUser()
{
	return UserPool.Alloc();
}

void CLobbyServer::freeUser(st_User* user)
{
	user->clear();
	UserPool.Free(user);
	return;
}




bool CLobbyServer::Redis_SetLoginStatus(const UID& uid, const Sid& SessionID)
{
	CRedis client;

	// redis 로그인 상태 등록
	string LoginUID = CombineStrings({ "Login", to_string(uid) }, ':');
	string IP_Port_SessionID = CombineStrings({ ServerSet.IP_char, to_string(ServerSet.Port), to_string(SessionID)}, ':');

	if (!client.setnx(LoginUID, IP_Port_SessionID))
		return false;

	return true;
}

bool CLobbyServer::Redis_SetLoginStatus(const string& nickname, const Sid& SessionID)
{
	CRedis client;

	// redis 로그인 상태 등록
	string LoginNickname = CombineStrings({ "Login", nickname }, ':');
	string IP_Port_SessionID = CombineStrings({ ServerSet.IP_char, to_string(ServerSet.Port), to_string(SessionID) }, ':');

	if (!client.set(LoginNickname, IP_Port_SessionID))
		return false;

	return true;
}

void CLobbyServer::Redis_RemoveLoginStatus(const UID& uid, const string& Nickname)
{
	CRedis client;

	// redis 로그인 상태 삭제
	string LoginUID = CombineStrings({ "Login", to_string(uid) }, ':');
	client.del({ LoginUID });

	string LoginNickname = CombineStrings({ "Login", Nickname }, ':');
	client.del({ LoginNickname });

	return;
}

void CLobbyServer::Redis_RemoveLoginStatus(const string& Nickname)
{
	CRedis client;

	string LoginNickname = CombineStrings({ "Login", Nickname }, ':');
	client.del({ LoginNickname });

	return;
}

//UID CLobbyServer::Redis_GetUID(const string& nickname)
//{
//	CRedis client;
//
//	// redis 로그인 유저 uid 확인
//	string LoginNickname = CombineStrings({ "Login", nickname }, ':');
//	auto opt = client.get_as_integer(LoginNickname);
//	if (!opt.has_value())
//		return 0;
//
//	return opt.value();
//}

std::optional<st_LoginStatus> CLobbyServer::Redis_GetLoginStatus(const UID& uid)
{
	CRedis client;

	string LoginUID = CombineStrings({ "Login", to_string(uid) }, ':');
	auto opt = client.get_as_string(LoginUID);
	if (!opt.has_value())
		return std::nullopt;

	auto ip_port_sessionid = SplitString(opt.value(), ':');

	st_LoginStatus loginStatus = { ip_port_sessionid[0], (WORD)std::stoi(ip_port_sessionid[1]), std::stoull(ip_port_sessionid[2]) };
	return loginStatus;
}

std::optional<st_LoginStatus> CLobbyServer::Redis_GetLoginStatus(const string& nickname)
{
	CRedis client;

	string LoginUID = CombineStrings({ "Login", nickname }, ':');
	auto opt = client.get_as_string(LoginUID);
	if (!opt.has_value())
		return std::nullopt;

	auto ip_port_sessionid = SplitString(opt.value(), ':');

	st_LoginStatus loginStatus = { ip_port_sessionid[0], (WORD)std::stoi(ip_port_sessionid[1]), std::stoull(ip_port_sessionid[2]) };
	return loginStatus;
}

void CLobbyServer::Redis_GetLoginStatus(vector<st_Friend>& Friends)
{
	CRedis client;

	for (auto& Friend : Friends)
	{
		string key = CombineStrings({ "Login", Friend.nickname }, ':');
		if (client.exists({ key }))
			Friend.isOnline = true;
		else
			Friend.isOnline = false;
	}

	return;
}

DWORD64 CLobbyServer::Redis_SetToken(const UID& uid)
{
	CRedis client;

	const char* Token = "111111111111111111111111111111111111111111111111111111111111111";
	if (!client.setex(to_string(uid), 30, Token))
		return 0;

	return 0;
}

vector<st_ServerStatus> CLobbyServer::Redis_GetServerStatus()
{
	CRedis client;
	vector<st_ServerStatus> ServerStatus;

	// redis 조회
	string key = "ServerStatus";
	auto opt = client.hgetall(key);
	if (!opt.has_value())
		return ServerStatus;

	// list로 변형
	auto& StatusList = opt.value();
	for (auto& Status : StatusList)
	{
		auto list = SplitString(Status.field, ':');
		ServerStatus.push_back({ list[0], (WORD)std::stoi(list[1]), list[2], (BYTE)std::stoi(Status.value) });
	}

	return ServerStatus;
}

bool CLobbyServer::Redis_Task_SetDuplicationLogin(const UID& uid)
{
	CRedis client;

	auto opt_LoginStatus = Redis_GetLoginStatus(uid);
	if (!opt_LoginStatus.has_value())
		return false;

	st_LoginStatus& LoginStatus = opt_LoginStatus.value();

	string key = CombineStrings({ LoginStatus.IP, to_string(LoginStatus.Port), to_string(static_cast<int>(en_REDIS_TASK_TYPE::DUPLICATION_LOGIN))}, ':');
	if (!client.rpush(key, { to_string(LoginStatus.SessionID) }))
		return false;

	return true;
}

bool CLobbyServer::Redis_Task_SetWhisper(const string& SendNickname, const string& RecvNickname, const string& Chatting)
{
	CRedis client;

	auto opt = Redis_GetLoginStatus(RecvNickname);
	if (!opt.has_value())
		return false;

	st_LoginStatus& LoginStatus = opt.value();

	string lkey = CombineStrings(
		{ 
			LoginStatus.IP,
			to_string(LoginStatus.Port),
			to_string(static_cast<int>(en_REDIS_TASK_TYPE::WHISPER))
		},
		':');
	string value = CombineStrings(
		{
			to_string(LoginStatus.SessionID),
			SendNickname,
			Chatting
		},
		':');

	if (!client.rpush(lkey, { value }))
		return false;

	return true;
}

bool CLobbyServer::Redis_Task_SetAddFriend(const string& SendNickname, const string& RecvNickname)
{
	CRedis client;

	auto opt = Redis_GetLoginStatus(RecvNickname);
	if (!opt.has_value())
		return false;

	st_LoginStatus& LoginStatus = opt.value();

	string lkey = CombineStrings(
		{
			LoginStatus.IP,
			to_string(LoginStatus.Port),
			to_string(static_cast<int>(en_REDIS_TASK_TYPE::ADD_FRIEND))
		},
		':');
	string value = CombineStrings(
		{
			to_string(LoginStatus.SessionID),
			SendNickname
		},
		':');

	if (!client.rpush(lkey, { value }))
		return false;

	return true;
}

bool CLobbyServer::Redis_Task_SetAcceptAddFriend(const string& SendNickname, const string& RecvNickname)
{
	CRedis client;

	auto opt = Redis_GetLoginStatus(RecvNickname);
	if (!opt.has_value())
		return false;

	st_LoginStatus& LoginStatus = opt.value();

	string lkey = CombineStrings(
		{
			LoginStatus.IP,
			to_string(LoginStatus.Port),
			to_string(static_cast<int>(en_REDIS_TASK_TYPE::ACCEPT_ADD_FRIEND))
		},
		':');
	string value = CombineStrings(
		{
			to_string(LoginStatus.SessionID),
			SendNickname
		},
		':');

	if (!client.rpush(lkey, { value }))
		return false;

	return true;
}

vector<string> CLobbyServer::Redis_Task_GetAndDelete(const string& key)
{
	CRedis client;

	// redis 조회
	auto Tasks = client.lrange(key, 0, -1);	// 전체 조회
	if (Tasks.empty())
		return Tasks;

	// redis 삭제
	client.ltrim(key, Tasks.size(), -1);

	return Tasks;
}

void CLobbyServer::Redis_Task_Execute()
{
	// { IP:Port:TaskType , [ Task1, Task2, ... ] }

	for (auto& key : RedisTaskKeys)	// ( RedisTaskType, key )
	{
		switch (key.first) 
		{
		case en_REDIS_TASK_TYPE::WHISPER:
		{
			vector<string> TaskList = Redis_Task_GetAndDelete(key.second);

			// { SessionID :SendNickname :Chatting }
			for (auto& Task : TaskList)
			{
				vector<string> TaskElement = SplitString(Task, ':');

				CPacket packet;
				Packet_Init_SC_Send_Whisper(packet, TaskElement[1], TaskElement[2]);
				SendMSG(std::stoull(TaskElement[0]), packet );
			}
		}break;

		case en_REDIS_TASK_TYPE::ADD_FRIEND:
		{
			vector<string> TaskList = Redis_Task_GetAndDelete(key.second);

			// { SessionID :SendNickname }
			for (auto& Task : TaskList)
			{
				st_RedisTask_AddFriend* pTask = new st_RedisTask_AddFriend(Task);

				Param param;
				param.field.ptr = reinterpret_cast<LONG64>(pTask);
				param.field.idx = static_cast<int>(en_ROOM_TASK_TYPE::ADD_FRIEND);

				SendToRoomSession(pTask->SessionID, param);
			}
		}break;

		case en_REDIS_TASK_TYPE::ACCEPT_ADD_FRIEND:
		{
			vector<string> TaskList = Redis_Task_GetAndDelete(key.second);

			// { SessionID :SendNickname }
			for (auto& Task : TaskList)
			{
				st_RedisTask_AcceptAddFriend* pTask = new st_RedisTask_AcceptAddFriend(Task);

				Param param;
				param.field.ptr = reinterpret_cast<LONG64>(pTask);
				param.field.idx = static_cast<int>(en_ROOM_TASK_TYPE::ACCEPT_ADD_FRIEND);

				SendToRoomSession(pTask->SessionID, param);
			}
		}break;

		case en_REDIS_TASK_TYPE::DUPLICATION_LOGIN:
		{
			vector<string> TaskList = Redis_Task_GetAndDelete(key.second);

			// { SessionID }
			for (auto& Task : TaskList)
				Disconnect(stoull(Task));
		}break;

		default:
			break;
		}
	}
}

void CLobbyServer::Redis_UpdateServerStatus()
{
	PerformanceNetworkData pnd;
	GetPND(pnd);

	CRedis client;

	string status;
	if (pnd.AcceptTotalCount >= 100)
		status = to_string(static_cast<int>(en_SERVER_STATUS_TYPE::RED));
	else if (pnd.AcceptTotalCount >= 50)
		status = to_string(static_cast<int>(en_SERVER_STATUS_TYPE::ORANGE));
	else
		status = to_string(static_cast<int>(en_SERVER_STATUS_TYPE::GREEN));

	string key = "ServerStatus";
	string field = CombineStrings({
		ServerSet.IP_char, 
		to_string(ServerSet.Port),
		ServerName },
		':');

	client.hset(key, field, status);

	return;
}



// User*을 위한 커스텀 삭제자
//void CustomDeleter(st_User* ptr)
//{
//	CLobbyServer::UserPool.Free(ptr);
//	return;
//}