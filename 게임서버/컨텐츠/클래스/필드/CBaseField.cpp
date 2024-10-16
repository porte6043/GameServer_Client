#include "DB.h"
#include "CBaseField.h"
#include "LobbyServer.h"

// 매니저
#include "ItemManager.h"
#include "StoreManager.h"

// 전역 함수
#include "Query.h"
#include "PacketInit.h"
#include "StringUtilities.h"

// 구조체
#include "Character.h"
#include "ServerStatus.h"
#include "RedisTask.h"

// enum
#include "LobbyProtocol.h"
#include "RoomNumber.h"
#include "RedisTaskType.h"
#include "RoomTaskType.h"
#include "en_LobbyServer.h"

// DB
#include "CRedis.h"

CBaseField::CBaseField() : 
	DB_Write(1), DB_Read(3)
{
	Server = nullptr;
}

void CBaseField::OnSetNetwork(CWanServerRoom2* network)
{
	Server = static_cast<CLobbyServer*>(network);
	fieldName = getFieldName(GetRoomNo());
}

void CBaseField::OnRecv(DWORD64 SessionID, CPacket& packet)
{
	DWORD type;
	packet >> type;
	switch (type)
	{
	case en_PACKET_CS_LOBBY_REQ_MOVE_FIELD:
		Packet_Move_Field(SessionID, packet);
		break;
	case en_PACKET_CS_LOBBY_READY_FIELD:
		Packet_Ready_Field(SessionID, packet);
		break;
	case en_PACKET_CS_LOBBY_REQ_MOVE_CHANNEL:
		Packet_Move_Channel(SessionID, packet);
		break;
	case en_PACKET_CS_LOBBY_REQ_CHANNEL_LIST:
		Packet_Channel_List(SessionID, packet);
		break;

	case en_PACKET_CS_LOBBY_REQ_STORE_PRODUCT_LIST:
		Packet_Store_List(SessionID, packet);
		break;
	case en_PACKET_CS_LOBBY_REQ_BUY_STORE_ITEM:
		Packet_Buy_Store_Item(SessionID, packet);
		break;
	case en_PACKET_CS_LOBBY_REQ_SHOW_ME_THE_MONEY:
		Packet_Show_Me_The_Money(SessionID, packet);
		break;
	case en_PACKET_CS_LOBBY_REQ_USE_ITEM:
		Packet_Use_Item(SessionID, packet);
		break;

	case en_PACKET_CS_LOBBY_REQ_ADD_FRIEND:
		Packet_Add_Friend(SessionID, packet);
		break;
	case en_PACKET_CS_LOBBY_REQ_ACCEPT_ADD_FRIEND:
		Packet_Accept_Add_Friend(SessionID, packet);
		break;
	case en_PACKET_CS_LOBBY_REQ_FRIEND_LIST:
		Packet_Friend_List(SessionID, packet);
		break;
	case en_PACKET_CS_LOBBY_REQ_SEND_WHISPER:
		Packet_Send_Whishper(SessionID, packet);
		break;
	default:
		// 가상함수 호출
		Recv(SessionID, static_cast<en_PACEKT_LOBBY_TYPE>(type), packet);
		break;
	}
}

void CBaseField::OnEnterRoom(DWORD64 SessionID, Param param)
{
	st_User* user = reinterpret_cast<st_User*>(param.value);
	if (user == nullptr)
		return;
	std::cout << fieldName << " 입장[" << user->Nickname << "]" << std::endl;

	UserMap.insert({ SessionID, user });
	user->setField(static_cast<en_ROOM_NUMBER>(GetRoomNo()), this);
	DB_Write.RequestQuery(
		Query_Update_User_RoomNo(user->Nickname, user->roomNo),
		nullptr
	);

	CPacket packet;
	Packet_Init_Move_Field(packet, true, GetRoomNo(), fieldName);
	SendMSG(SessionID, packet);

	// 가상함수 호출
	EnterRoom(SessionID, param);

	return;
}

void CBaseField::OnLeaveRoom(DWORD64 SessionID)
{
	auto user = UserMap.at(SessionID);
	std::cout << fieldName << " 퇴장[" << user->Nickname << "]" << std::endl;
	// 가상함수 호출
	LeaveRoom(SessionID);

	UserMap.erase(SessionID);
	return;
}

void CBaseField::OnSessionKickOut(DWORD64 SessionID)
{

}

void CBaseField::OnSessionRelease(DWORD64 SessionID)
{
	// 가상함수 호출
	SessionRelease(SessionID);

	st_User* user = UserMap.at(SessionID);
	std::cout << fieldName << " 접속 종료[" << user->Nickname << "]" << std::endl;

	DB_Write.RequestLastQuery(
		[uid = user->uid, nickname = user->Nickname, this](MYSQL_RES* mysql_res, unsigned long long errorno)
		{
			Server->Redis_RemoveLoginStatus(uid, nickname);
		});

	UserMap.erase(SessionID);
	Server->freeUser(user);
	return;
}

void CBaseField::OnRecvToRoom(Param param)
{
	en_ROOM_TASK_TYPE Type = static_cast<en_ROOM_TASK_TYPE>(param.field.idx);

	switch (Type) {
	case en_ROOM_TASK_TYPE::ADD_FRIEND:
	{
		// 친구 추가 요청
		st_RedisTask_AddFriend* pTask = reinterpret_cast<st_RedisTask_AddFriend*>(param.field.ptr);
		auto iter = UserMap.find(pTask->SessionID);
		if (iter == UserMap.end())
			break;

		auto user = iter->second;
		user->AddFriends.push_back(pTask->SendNickname);

		CPacket packet = CPacket::Alloc();
		Packet_Init_Req_Friend(packet, pTask->SendNickname);
		SendMSG(user->SessionID, packet);
		delete pTask;
	}break;

	case en_ROOM_TASK_TYPE::ACCEPT_ADD_FRIEND:
	{
		// 친구 추가 승인
		st_RedisTask_AcceptAddFriend* pTask = reinterpret_cast<st_RedisTask_AcceptAddFriend*>(param.field.ptr);
		auto iter = UserMap.find(pTask->SessionID);
		if (iter == UserMap.end())
			break;

		auto user = iter->second;
		user->Friends.push_back({ pTask->SendNickname, false });

		CPacket packet = CPacket::Alloc();
		Packet_Init_Accept_Friend(packet, pTask->SendNickname);
		SendMSG(user->SessionID, packet);
		delete pTask;
	}break;

	default:
		break;
	}

	// 가상함수 호출
	RecvToRoom(param);

	return;
}

void CBaseField::OnUpdate()
{
	// DB Respond
	DB_Write.RespondQuery();
	DB_Read.RespondQuery();

	// 가상함수 호출
	Update();
}

bool CBaseField::sendToRoom(const Sid& sessionID, Param param)
{
	DWORD roomNo = Server->GetRoomNo(sessionID);
	if (roomNo == INVALID_ROOMNO)
		return false;

	Server->SendToRoom(roomNo, param);
	return true;
}

bool CBaseField::MoveRoom(const Sid& SessionID, const DWORD& RoomNo, st_User* user)
{
	DB_Write.RequestLastQuery(
		[SessionID, RoomNo, user, this](MYSQL_RES* mysql_res, unsigned int errorno)
		{
			if(!ChangeRoom(SessionID, RoomNo, user))	
			{
				CPacket packet = CPacket::Alloc();
				Packet_Init_Move_Field(packet, false, GetRoomNo(), fieldName);
				SendMSG(SessionID, packet);
				return;
			}
		});

	return true;
}





// packet
//------------------------------------------------------------------------------------------------------

void CBaseField::Packet_Move_Field(const Sid& SessionID, CPacket& packet)
{
	DWORD RoomNo;
	packet >> RoomNo;
	if (packet.GetUseSize() != 0)
	{
		Disconnect(SessionID);
		return;
	}

	auto user = UserMap.at(SessionID);
	MoveRoom(SessionID, RoomNo, user);
	return;
}
void CBaseField::Packet_Ready_Field(const Sid& SessionID, CPacket& packet)
{
	// Serter 삽입
	// 캐릭터 생성 패킷 전송
	// 다른 캐릭터 생성 패킷 전송 등등의 작업을 합니다.
}
void CBaseField::Packet_Move_Channel(const Sid& SessionID, CPacket& packet)
{
	char ip[16];
	WORD port;
	packet.GetData(ip, 16);
	packet >> port;
	if (packet.GetUseSize() != 0)
	{
		Disconnect(SessionID);
		return;
	}

	SuspendRecv(SessionID);

	auto user = UserMap.at(SessionID);
	DB_Write.RequestLastQuery(
		[uid = user->uid, nickname = user->Nickname, SessionID, this](MYSQL_RES* mysql_res, unsigned long long errorno)
		{
			Server->Redis_RemoveLoginStatus(uid, nickname);
			
			const char* Token = "111111111111111111111111111111111111111111111111111111111111111";
			Server->Redis_SetToken(uid);

			CPacket Packet;
			Packet_Init_Move_Channel(Packet, Token);
			SendMSG_Disconnect(SessionID, Packet, 30);

		});

	return;
}
void CBaseField::Packet_Channel_List(const Sid& SessionID, CPacket& packet)
{
	// 1. redis 서버 상태에 가서 모든 같은 월드의 채널을 전부 가져온다
	// 2. 보낸다.
	if (packet.GetUseSize() != 0)
	{
		Disconnect(SessionID);
		return;
	}

	auto SeverStatusList = Server->Redis_GetServerStatus();

	CPacket Packet = CPacket::Alloc();
	Packet_Init_Channel_List(Packet, SeverStatusList);
	SendMSG(SessionID, Packet);

	return;
}

//------------------------------------------------------------------------------------------------------

// 상점 packet 처리

void CBaseField::Packet_Store_List(const Sid& SessionID, CPacket& packet)
{
	BYTE storeID;
	packet >> storeID;
	if (packet.GetUseSize() != 0)
	{
		Disconnect(SessionID);
		return;
	}

	CStoreManager storeManager;
	auto StoreList = storeManager.getStoreList(storeID);
	if (StoreList.size() == 0)
	{
		Disconnect(SessionID);
		return;
	}

	CPacket Packet;
	Packet_Init_Store_List(Packet, StoreList);
	SendMSG(SessionID, Packet);

	return; 
}
void CBaseField::Packet_Buy_Store_Item(const Sid& SessionID, CPacket& packet)
{
	BYTE storeID;
	DWORD productID;
	DWORD quantity;
	packet >> storeID >> productID >> quantity;
	if (packet.GetUseSize() != 0)
	{
		Disconnect(SessionID);
		return;
	}
	
	CStoreManager storeManager;
	auto& user = UserMap.at(SessionID);

	storeManager.buy(storeID, productID, quantity, user);
	return;
}
void CBaseField::Packet_Show_Me_The_Money(const Sid& SessionID, CPacket& packet)
{
	BYTE moneyID;
	packet >> moneyID;
	if (packet.GetUseSize() != 0)
	{
		Disconnect(SessionID);
		return;
	}

	auto user = UserMap.at(SessionID);

	DWORD finalMoneyQuantity = user->inventory.addMoney( moneyID, 10000 );
	DB_Write.RequestQuery(
		Query_Update_User_Money(user->Nickname, moneyID, finalMoneyQuantity),
		nullptr
	);

	CPacket Packet;
	Packet_Init_Show_Me_The_Money(Packet, moneyID, finalMoneyQuantity);
	SendMSG(SessionID, Packet);

	return;
}
void CBaseField::Packet_Use_Item(const Sid& SessionID, CPacket& packet)
{
	DWORD itemID;
	packet >> itemID;
	if (packet.GetUseSize() != 0)
	{
		Disconnect(SessionID);
		return;
	}

	auto user = UserMap.at(SessionID);

	CItemManager itemManager;
	itemManager.useItem(itemID, user);

	return;
}

//------------------------------------------------------------------------------------------------------

void CBaseField::Packet_Add_Friend(const Sid& SessionID, CPacket& packet)
{
	char recvNickname[LEN_NICKNAME];
	packet.GetData(recvNickname, LEN_NICKNAME);
	if (packet.GetUseSize() != 0)
	{
		Disconnect(SessionID);
		return;
	}

	string SendNickname = UserMap.at(SessionID)->Nickname;
	string RecvNickname = recvNickname;

	// RecvNickname이 존재하는 캐릭터인지 확인
	DB_Read.RequestQuery(
		Query_Select_Nickname(RecvNickname),
		[SessionID, SendNickname, RecvNickname, this](MYSQL_RES* mysql_res, unsigned int errorno)
		{
			if (errorno != 0)
				return;

			if (mysql_res->row_count == 0)
			{
				// 존재하지않는 닉네임
				CPacket packet = CPacket::Alloc();
				Packet_Init_Add_Friend(packet, false);
				SendMSG(SessionID, packet);
			}
			else
			{
				DB_Write.RequestQuery(
					Query_Insert_AddFriendList(RecvNickname, SendNickname),
					[SessionID, SendNickname, RecvNickname, this](MYSQL_RES* mysql_res, unsigned int errorno)
					{
						if (errorno != 0)
						{
							// Insert 실패
							CPacket packet = CPacket::Alloc();
							Packet_Init_Add_Friend(packet, false);
							SendMSG(SessionID, packet);

							return;
						}
						else
						{
							// Insert 성공
							Server->Redis_Task_SetAddFriend(SendNickname, RecvNickname);

							CPacket packet = CPacket::Alloc();
							Packet_Init_Add_Friend(packet, true);
							SendMSG(SessionID, packet);
						}
						mysql_free_result(mysql_res);
					});
				
			}
			mysql_free_result(mysql_res);
		}
	);

	return;
}
void CBaseField::Packet_Accept_Add_Friend(const Sid& SessionID, CPacket& packet)
{
	char FriendNickname[LEN_NICKNAME];
	packet.GetData(FriendNickname, LEN_NICKNAME);
	if (packet.GetUseSize() != 0)
	{
		Disconnect(SessionID);
		return;
	}
	
	// user->AddFriend List에 있는 Friend인지 확인
	// 트렌젝션
	// 친추목록 리스트 삭제
	// 친구목록 리스트 등록 * 2
	// 커밋
	// 승락한 유저에게 응답 보내기
	// 승락된 유저가 로그인 중이면 그 쪽에도 승락 메시지 전송
	
	auto user = UserMap.at(SessionID);
	auto iter = std::find(user->AddFriends.begin(), user->AddFriends.end(), FriendNickname);
	if (iter == user->AddFriends.end())
		Disconnect(SessionID);

	
	DB_Write.RequestTransactionQuery(
		{
			Query_Delete_AddFriendList(user->Nickname, FriendNickname),
			Query_Insert_FriendList(user->Nickname, FriendNickname),
			Query_Insert_FriendList(FriendNickname, user->Nickname),
		},
		[this, SessionID, FriendNickname](MYSQL_RES* mysql_res, unsigned int errorno)
		{
			auto iter = UserMap.find(SessionID);
			auto user = iter->second;

			if (errorno == 0)
			{
				// 성공
				user->AddFriends.erase( std::remove(user->AddFriends.begin(), user->AddFriends.end(), FriendNickname), user->AddFriends.end() );
				user->Friends.push_back({ FriendNickname, false });
				Server->Redis_Task_SetAcceptAddFriend(user->Nickname, FriendNickname);

				CPacket packet = CPacket::Alloc();
				Packet_Init_Accept_AddFriend(packet, true, FriendNickname);
				SendMSG(SessionID, packet);
			}
			else
			{
				// 실패
				CPacket packet = CPacket::Alloc();
				Packet_Init_Accept_AddFriend(packet, false, {});
				SendMSG(SessionID, packet);
			}
		}
	);

	return;
}
void CBaseField::Packet_Friend_List(const Sid& SessionID, CPacket& packet)
{
	if (packet.GetUseSize() != 0)
	{
		Disconnect(SessionID);
		return;
	}

	auto& user = UserMap.at(SessionID);
	Server->Redis_GetLoginStatus(user->Friends);

	CPacket Packet = CPacket::Alloc();
	Packet_Init_Friend_List(Packet, user->Friends);
	SendMSG(SessionID, Packet);
	return;
}
void CBaseField::Packet_Send_Whishper(const Sid& SessionID, CPacket& packet)
{
	char RecvNickname[LEN_NICKNAME];
	char Chatting[LEN_CHATTHING];
	packet.GetData(RecvNickname, LEN_NICKNAME);
	packet.GetData(Chatting, LEN_CHATTHING);
	if (packet.GetUseSize() != 0)
	{
		Disconnect(SessionID);
		return;
	}

	auto& user = UserMap.at(SessionID);
	bool status = Server->Redis_Task_SetWhisper(user->Nickname, RecvNickname, Chatting);

	CPacket Packet = CPacket::Alloc();
	Packet_Init_Send_Whisper(Packet, status);
	SendMSG(SessionID, Packet);

	return;
}

//--------------------------------------------------------------------2----------------------------------