#include "Lobby.h"

// ����
#include "LobbyServer.h"
#include "���� ���̺귯��/TlsProfiling.h"

// ���� �Լ�
#include "Query.h"
#include "PacketInit.h"
#include "StringUtilities.h"

// ���� �� ��������
#include "LobbyProtocol.h"
#include "RoomNumber.h"
#include "RedisTaskType.h"
#include "en_LobbyServer.h"

// ����ü
#include "Character.h"
#include "RedisTask.h"

// DB
#include "CRedis.h"


CLobby::CLobby() : db(3)
{
	Server = nullptr;
}
CLobby::~CLobby()
{

}

void CLobby::OnSetNetwork(CWanServerRoom2* network)
{
	Server = static_cast<CLobbyServer*>(network);
}

void CLobby::OnRecv(DWORD64 SessionID, CPacket& packet)
{
	int Type;
	packet >> Type;
	switch (Type) {
	case en_PACKET_CS_LOBBY_REQ_LOGIN:
	{
		UID uid;
		string Token;
		packet >> uid;
		packet.GetData(Token, LEN_SESSIONKEY);
		if (packet.GetUseSize() != 0)
		{
			Disconnect(SessionID);
			return;
		}

		// token �˻�
		CRedis client;
		auto opt = client.get_as_string(std::to_string(uid));
		if (!opt.has_value())
		{
			Disconnect(SessionID);
			return;
		}

		string token = opt.value();
		if (token != Token)
		{
			Disconnect(SessionID);
			return;
		}

		// uid �Է�
		auto& user = UserMap.at(SessionID);
		user->uid = uid;

		// redis �α��� ���� ���
		if (!Server->Redis_SetLoginStatus(user->uid, user->SessionID))
		{
			DuplicationUser.push_back({ SessionID, GetTickCount64() + 50000});
			Server->Redis_Task_SetDuplicationLogin(user->uid);
			return;
		}

		// DB ĳ���� ����Ʈ ��û
		db.RequestQuery(
			Query_Select_CharacterList(uid),
			[SessionID, this](MYSQL_RES* sql_res, unsigned int errorno)
			{
				// ���� ����
				if (errorno != 0)
				{
					Disconnect(SessionID);
					mysql_free_result(sql_res);
					return;
				}

				// ���� ����
				auto user = FindUser(SessionID);
				if (user == nullptr)
				{
					mysql_free_result(sql_res);
					return;
				}

				MYSQL_ROW sql_row;
				while ((sql_row = mysql_fetch_row(sql_res)) != NULL)
				{
					st_CharacterSeq CharSeq = { sql_row[0], (BYTE)std::stoi(sql_row[1]) };
					user->Characters.push_back(CharSeq);
				}
				mysql_free_result(sql_res);

				CPacket packet;
				Packet_Init_Login(packet, user->Characters);
				SendMSG(SessionID, packet);
				return;
			});
	}break;

	case en_PACKET_CS_LOBBY_REQ_LOGIN_OTHER_CHANNEL:
	{
		UID uid;
		string Token;
		string Nickname;
		packet >> uid;
		packet.GetData(Token, LEN_SESSIONKEY);
		packet.GetData(Nickname, LEN_NICKNAME);
		if (packet.GetUseSize() != 0)
		{
			Disconnect(SessionID);
			return;
		}

		// token �˻�
		CRedis redis;
		auto opt = redis.get_as_string(std::to_string(uid));
		if (!opt.has_value())
		{
			Disconnect(SessionID);
			return;
		}

		string token = opt.value();
		if (token != Token)
		{
			Disconnect(SessionID);
			return;
		}

		// uid �Է�
		auto& user = UserMap.at(SessionID);
		user->uid = uid;

		// redis �α��� ���� ���
		if (!Server->Redis_SetLoginStatus(user->uid, user->SessionID))
		{
			DuplicationUser.push_back({ SessionID, GetTickCount64() + 50000 });
			Server->Redis_Task_SetDuplicationLogin(user->uid);
			return;
		}

		// DB ĳ���� ����Ʈ ��û
		db.RequestQuery(
			Query_Select_CharacterList(uid),
			[SessionID, this](MYSQL_RES* sql_res, unsigned int errorno)
			{
				// ���� ����
				if (errorno != 0)
				{
					Disconnect(SessionID);
					mysql_free_result(sql_res);
					return;
				}

				// ���� ����
				auto user = FindUser(SessionID);
				if (user == nullptr)
				{
					mysql_free_result(sql_res);
					return;
				}

				MYSQL_ROW sql_row;
				while ((sql_row = mysql_fetch_row(sql_res)) != NULL)
				{
					st_CharacterSeq CharSeq = { sql_row[0], (BYTE)std::stoi(sql_row[1]) };
					user->Characters.push_back(CharSeq);
				}
				mysql_free_result(sql_res);

				return;
			});

		// DB ������ ĳ���� ���� ��û
		db.RequestQuery(
			Query_Select_Character(Nickname),
			[SessionID, this](MYSQL_RES* sql_res, unsigned int errorno)
			{
				// ���� ����
				if (errorno != 0)
				{
					LOG(L"LobbyServer", CLog::LEVEL_ERROR, L"[Query_Select_Character Errorno=%ld]", errorno);
					Disconnect(SessionID);
					return;
				}

				auto user = FindUser(SessionID);
				if (user == nullptr)
				{
					mysql_free_result(sql_res);
					return;
				}


				MYSQL_ROW sql_row = mysql_fetch_row(sql_res);

				user->Nickname = sql_row[0];
				user->roomNo = static_cast<en_ROOM_NUMBER>(std::stoul(sql_row[1]));
				string StatsList = sql_row[2];
				string MoneyList = sql_row[3];
				string ItemList = sql_row[4];
				string FriendList = sql_row[5];
				string AddFriendList = sql_row[6];
				mysql_free_result(sql_res);

				auto Statses = SplitString(StatsList, ',');
				for (auto& element : Statses) // { stats_id, stats }
				{
					auto Status = SplitString(element, ':');
					user->stats.setStats(static_cast<BYTE>(std::stoi(Status[0])), std::stoul(Status[1]));
				}

				auto Moneys = SplitString(MoneyList, ',');
				for (auto& element : Moneys)	// { money_id, money }
				{
					auto Money = SplitString(element, ':');
					user->inventory.setMoney(std::stoi(Money[0]), std::stoi(Money[1]));
				}

				auto Items = SplitString(ItemList, ',');
				CItemManager itemManager;
				for (auto& element : Items)		// { itemid, quantity, sequence }
				{
					auto Item = SplitString(element, ':');
					user->inventory.setItem(
						itemManager.getItemByRef(std::stoul(Item[0])).value().get(),
						std::stoi(Item[1]),
						std::stoi(Item[2]));
				}

				auto Friends = SplitString(FriendList, ',');
				for (auto& Friend : Friends)	// { friend_nickname }
				{
					user->Friends.push_back({ Friend , false });
				}

				auto AddFriends = SplitString(AddFriendList, ',');
				for (auto& AddFriend : AddFriends)	// { requset_nickname }
				{
					user->AddFriends.push_back(AddFriend);
				}

				Server->Redis_SetLoginStatus(user->Nickname, SessionID);

				ChangeRoom(SessionID, static_cast<DWORD>(user->roomNo), user);
			});
	}break;

	case en_PACKET_CS_LOBBY_REQ_SELECT_CHARACTER:
	{
		char CharacterName[LEN_NICKNAME];
		packet.GetData(CharacterName, LEN_NICKNAME);
		if (packet.GetUseSize() != 0)
		{
			Disconnect(SessionID);
			return;
		}

		// �����ϴ� character���� Ȯ��
		auto& user = UserMap.at(SessionID);
		auto iter = std::find_if(
			user->Characters.begin(),
			user->Characters.end(),
			[&CharacterName](st_CharacterSeq CharacterSeq)
			{ return CharacterSeq.nickname.compare(CharacterName) == 0; });
		if (iter == user->Characters.end())
		{
			Disconnect(SessionID);
			return;
		}

		// DB ������ ĳ���� ���� ��û
		db.RequestQuery(
			Query_Select_Character(CharacterName),
			[SessionID, this](MYSQL_RES* sql_res, unsigned int errorno)
			{
				// ���� ����
				if (errorno != 0)
				{
					LOG(L"LobbyServer", CLog::LEVEL_ERROR, L"[Query_Select_Character Errorno=%ld]", errorno);
					Disconnect(SessionID);
					return;
				}
		
				auto user = FindUser(SessionID);
				if (user == nullptr)
				{
					mysql_free_result(sql_res);
					return;
				}

				
				MYSQL_ROW sql_row = mysql_fetch_row(sql_res);
					
				user->Nickname = sql_row[0];
				user->roomNo = static_cast<en_ROOM_NUMBER>(std::stoul(sql_row[1]));
				string StatsList = sql_row[2];
				string MoneyList = sql_row[3];
				string ItemList = sql_row[4];
				string FriendList = sql_row[5];
				string AddFriendList = sql_row[6];
				mysql_free_result(sql_res);

				auto Statses = SplitString(StatsList, ',');
				for (auto& element : Statses) // { stats_id, stats }
				{
					auto Status = SplitString(element, ':');
					user->stats.setStats(static_cast<BYTE>(std::stoi(Status[0])), std::stoul(Status[1]));
				}

				auto Moneys = SplitString(MoneyList, ',');
				for (auto& element : Moneys)	// { money_id, money }
				{
					auto Money = SplitString(element, ':');
					user->inventory.setMoney(std::stoi(Money[0]), std::stoi(Money[1]) );
				}

				auto Items = SplitString(ItemList, ',');
				CItemManager itemManager;
				for (auto& element : Items)		// { itemid, quantity, sequence }
				{
					auto Item = SplitString(element, ':');
					user->inventory.setItem(
						itemManager.getItemByRef(std::stoul(Item[0])).value().get(),
						std::stoi(Item[1]),
						std::stoi(Item[2]));
				}

				auto Friends = SplitString(FriendList, ',');
				for (auto& Friend : Friends)	// { friend_nickname }
				{
					user->Friends.push_back({ Friend , false });
				}

				auto AddFriends = SplitString(AddFriendList, ',');
				for (auto& AddFriend : AddFriends)	// { requset_nickname }
				{
					user->AddFriends.push_back(AddFriend);
				}

				
				CPacket packet;
				Packet_Init_SelectCharacter(packet, user);
				SendMSG(SessionID, packet);

				Server->Redis_SetLoginStatus(user->Nickname, SessionID);

				ChangeRoom(SessionID, static_cast<DWORD>(user->roomNo), user);
			});
	}break;

	case en_PACKET_CS_LOBBY_REQ_CHARACTERLIST:
	{
		auto user = UserMap.at(SessionID);

		auto optLoginStatus = Server->Redis_GetLoginStatus(user->uid);
		if (!optLoginStatus.has_value())
		{
			Disconnect(SessionID);
			return;
		}

		// DB ĳ���� ����Ʈ ��û
		db.RequestQuery(
			Query_Select_CharacterList(user->uid),
			[SessionID, this](MYSQL_RES* sql_res, unsigned int errorno)
			{
				// ���� ����
				if (errorno != 0)
				{
					CPacket packet;
					Packet_Init_Character_List(packet, {});
					SendMSG(SessionID, packet);
					return;
				}

		// ���� ����
		auto user = FindUser(SessionID);
		if (user == nullptr)
		{
			mysql_free_result(sql_res);
			return;
		}

		MYSQL_ROW sql_row;
		user->Characters.clear();
		while ((sql_row = mysql_fetch_row(sql_res)) != NULL)
		{
			st_CharacterSeq CharSeq = { sql_row[0], (BYTE)std::stoi(sql_row[1]) };
			user->Characters.push_back(CharSeq);
		}
		mysql_free_result(sql_res);

		CPacket packet;
		Packet_Init_Character_List(packet, user->Characters);
		SendMSG(SessionID, packet);
		return;
			});
	}break;

	case en_PACKET_CS_LOBBY_REQ_AVAILABLE_NICKNAME:
	{
		char Nickname[LEN_NICKNAME];
		packet.GetData(Nickname, LEN_NICKNAME);
		if (packet.GetUseSize() != 0)
		{
			Disconnect(SessionID);
			return;
		}

		// DB ��� ���� �г��� Ȯ�� ��û
		db.RequestQuery(
			Query_Select_Nickname(Nickname),
			[SessionID, this](MYSQL_RES* sql_res, unsigned int errorno)
			{
				bool ret = false;

				// ���� ����
				if (errorno != 0)
					LOG(L"LobbyServer", CLog::LEVEL_ERROR, L"[Query_Select_Nickname Errorno=%ld]", errorno);
				
				if (sql_res->row_count == 0)
					ret = true;
				mysql_free_result(sql_res);
				
				CPacket packet;
				Packet_Init_AvailableNickname(packet, ret);
				SendMSG(SessionID, packet);
			});
	}break;

	case en_PACKET_CS_LOBBY_REQ_CREATE_CHARACTER:
	{
		char Nickname[LEN_NICKNAME];
		packet.GetData(Nickname, LEN_NICKNAME);
		if (packet.GetUseSize() != 0)
		{
			Disconnect(SessionID);
			return;
		}

		auto user = UserMap.at(SessionID);

		// DB ĳ���� ���� ��û
		db.RequestTransactionQuery(
			{ 
			Query_Insert_CreateCharacter(user->uid, Nickname, user->Characters.size() + 1),
			Quert_Insert_CreateCharacter_Stats(Nickname),
			Query_Insert_CreateCharacter_Money(Nickname),
			Query_Insert_CreateCharacter_Position(Nickname)
			},
			[SessionID, this](MYSQL_RES* sql_res, unsigned int errorno)
			{
				// ���� ����
				if (errorno != 0)
				{
					CPacket packet;
					Packet_Init_Create_Character(packet, false);
					SendMSG(SessionID, packet);
				}
				else
				{
					CPacket packet;
					Packet_Init_Create_Character(packet, true);
					SendMSG(SessionID, packet);
				}
				mysql_free_result(sql_res);
			});
	}break;

	default:
		break;
	}
}

void CLobby::OnEnterRoom(DWORD64 SessionID, Param param)
{
	std::cout << "�κ� ����" << std::endl;
	if(param.value == 0)
	{
		// ó�� �����ϴ� ����
		st_User* user = Server->allocUser();
		user->clear();

		// user �ʱ�ȭ
		user->SessionID = SessionID;
		user->LastTimeTick = GetTickCount64();
		user->roomNo = en_ROOM_NUMBER::Lobby;

		UserMap.insert({ SessionID, user });
		return;
	}
	else
	{
		// �̹� �����ϰ� �ִ� ����
		st_User* user = reinterpret_cast<st_User*>(param.field.ptr);
		Server->Redis_RemoveLoginStatus(user->Nickname);

		// �ʱ�ȭ
		user->LastTimeTick = GetTickCount64();
		user->roomNo = en_ROOM_NUMBER::Lobby;
		user->pField = nullptr;
		user->Nickname.clear();
		user->stats.clear();
		user->inventory.clear();
		user->Friends.clear();
		user->AddFriends.clear();

		UserMap.insert({ SessionID, user });

		CPacket packet;
		Packet_Init_Move_Field(packet, true, GetRoomNo(), getFieldName(GetRoomNo()));
		SendMSG(SessionID, packet);
		return;
	}
}

void CLobby::OnLeaveRoom(DWORD64 SessionID)
{
	std::cout << "�ʵ� �̵�" << std::endl;
	UserMap.erase(SessionID);
	return;
}

void CLobby::OnSessionKickOut(DWORD64 SessionID)
{
	// Room�� �ı� ���� �� ���� �ִ� Session�� ó���� �մϴ�.
	// �ش� Session�� �ٸ� Room���� �ű���� ChangeRoom�� ����մϴ�.
	// ���� �ش� �Լ����� �ƹ��͵� ���ϸ� ���� �ִ� Session���� ���� Disconnect �˴ϴ�.
}

void CLobby::OnSessionRelease(DWORD64 SessionID)
{
	std::cout << "�κ� ���� ����" << std::endl;
	auto user = UserMap.at(SessionID);

	UserMap.erase(SessionID);
	Server->Redis_RemoveLoginStatus(user->uid, user->Nickname);
	Server->freeUser(user);

	return;
}

void CLobby::OnRecvToRoom(Param param)
{
	return;
}

void CLobby::OnUpdate()
{
	db.RespondQuery();

	// DuplicationUser  ���鼭 üũ
	for (auto iter = DuplicationUser.begin(); iter != DuplicationUser.end(); )
	{
		Sid& SessionID = iter->first;
		INT64& TimeOut = iter->second;

		auto user = FindUser(SessionID);
		if (user == nullptr)
		{
			// ���� ���� ���� ���
			iter = DuplicationUser.erase(iter);
			continue;
		}

		if (GetTickCount64() > TimeOut)
		{
			// time out�� �ɸ� ���
			iter = DuplicationUser.erase(iter);
			continue;
		}

		auto opt = Server->Redis_GetLoginStatus(user->uid);
		if (opt.has_value())
		{
			++iter;
			continue;
		}

		// �α��� �õ�
		if (Server->Redis_SetLoginStatus(user->uid, user->SessionID))
		{
			// �α��� ����

			// DB ĳ���� ����Ʈ ��û
			db.RequestQuery(
				Query_Select_CharacterList(user->uid),
				[SessionID, this](MYSQL_RES* sql_res, unsigned int errorno)
				{
					// ���� ����
					if (errorno != 0)
					{
						Disconnect(SessionID);
						mysql_free_result(sql_res);
						return;
					}

					// ���� ����
					auto user = FindUser(SessionID);
					if (user == nullptr)
					{
						mysql_free_result(sql_res);
						return;
					}

					MYSQL_ROW sql_row;
					while ((sql_row = mysql_fetch_row(sql_res)) != NULL)
					{
						st_CharacterSeq CharSeq = { sql_row[0], (BYTE)std::stoi(sql_row[1]) };
						user->Characters.push_back(CharSeq);
					}
					mysql_free_result(sql_res);

					CPacket packet;
					Packet_Init_Login(packet, user->Characters);
					SendMSG(SessionID, packet);
					return;
				});

			iter = DuplicationUser.erase(iter);
		}
	}

	return;
}



// =================================================================
st_User* CLobby::FindUser(const Sid& SessionID)
{
	auto iter = UserMap.find(SessionID);
	if (iter == UserMap.end())
		return nullptr;
	else
		return iter->second;
}