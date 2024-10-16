#pragma once
#include "네트워크/CWanClient.h"
#include "공용 라이브러리/CCsvParser.h"
#include "GameServerInfo.h"
#include "RoomManager.h"
#include "Character.h"
#include "Data.h"

#include <algorithm>
#include <functional>	
#include <iostream>	
#include <vector>	
using std::function;
using std::vector;
using std::cout;
using std::cin;
using std::endl;

class GameClient : public CWanClient
{
public:
	// 초기화 변수
	wstring ip;
	WORD	port;
	BYTE	packetCode;
	BYTE	packetKey;

	// 로그인에 필요한 변수
	st_GameServerInfo			gameServerInfo;
	string						createNickname;


	// 게임에 필요한 변수
	st_Character						character;
	bool								isMoveChannel;
	st_Data								data;
	function<void(string)>				currentScene;
	string								currentCmdPath;

	queue<CPacket>						logicQueue;
	SRWLOCK								SRW_logicQueue;

	queue<std::pair<function<void(string)>, string>>		cmdQueue;
	SRWLOCK													SRW_cmdQueue;

	GameClient();

	// 서버와 연결됐을 때
	void OnConnectServer() override;

	// 서버와 연결이 끊어졌을 때
	void OnDisconnectServer() override;

	// Message 수신 완료 후 Contents Packet 처리를 합니다.
	void OnRecv(CPacket& packet) override;

	void login();

	void directLogin();

	void update();

	void cmd();

	// packet res
	void Packet_Login(CPacket& packet);
	void Packet_Login_Other_Channel(CPacket& packet);
	void Packet_Select_Character(CPacket& packet);
	void Packet_CharacterList(CPacket& packet);
	void Packet_Available_Nickname(CPacket& packet);
	void Packet_Create_Character(CPacket& packet);
	void Packet_Move_Field(CPacket& packet);
	void Packet_Move_Channel(CPacket& packet);
	void Packet_Channel_List(CPacket& packet);
	void Packet_Store_Product_List(CPacket& packet);
	void Packet_Buy_Store_Item(CPacket& packet);
	void Packet_Show_Me_The_Money(CPacket& packet);
	void Packet_Use_Item_Failed(CPacket& packet);
	void Packet_Use_Item_Stats_Box(CPacket& packet);
	void Packet_Add_Friend(CPacket& packet);
	void Packet_Req_Friend(CPacket& packet);
	void Packet_Accept_Friend(CPacket& packet);
	void Packet_Accept_Add_Friend(CPacket& packet);
	void Packet_Friend_List(CPacket& packet);
	void Packet_Send_Whisper(CPacket& packet);
	void Packet_Req_Send_Whishper(CPacket& packet);

	


	// Req To Server
	void Login();
	void AvailableNickname(string nickname);
	void CreateCharacter(string nickname);
	void CharacterList();
	void Select_Character(string nickname);

	void MoveField(DWORD roomNo);
	void MoveChennel(string serverName, string ip, WORD port);
	void ChannelList();

	void StoreProductList(BYTE storeID);
	void BuyStoreItem(BYTE storeID, DWORD itemID, DWORD quantity);
	void ShowMeTheMoney();
	void UseItem(DWORD itemID);

	void AddFriend(string nickname);
	void AcceptAddFriend(string nickname);
	void FriendList();
	void SendWhishper(string recvNickname, string chatting);

	// scene
	void setScene(function<void(string)> scene);
	void setCmd(string cmd);

	void Scene_Login(string cmd);
	void Scene_CreateCharacter(string cmd);
	void Scene_CreateCharacterStatus(string cmd);
	void Scene_CreateCharacter_Available_NicknameStatus(string cmd);
	void Scene_SelectCharacter(string cmd);

	void Scene_Field(string cmd);
	void Scene_MoveField(string cmd);
	void Scene_MoveChannel(string cmd);
	void Scene_ChannelList(string cmd);
	void Scene_StoreProductList(string cmd);
	void Scene_BuyItemStatus(string cmd);
	void Scene_inventory(string cmd);
	void Scene_inventoryRegularItemList(string cmd);
	void Scene_inventoryUsableItemList(string cmd);
	void Scene_inventoryMoneyList(string cmd);
	void Scene_UseItem(string cmd);
	void Scene_ShowMeTheMoney(string cmd);
	void Scene_Friend(string cmd);
	void Scene_FriendList(string cmd);
	void Scene_AddFriendList(string cmd);
	void Scene_AcceptAddFriendStatus(string cmd);
	void Scene_AddFriend(string cmd);
	void Scene_AddFriendStatus(string cmd);
	void Scene_SendFriendWhishper(string cmd);
	void Scene_WhishperStatus(string cmd);
	void Scene_Whisper(string cmd);
	void Scene_SendWhisper(string cmd);
	void Scene_CharacterStats(string cmd);
	
	


};