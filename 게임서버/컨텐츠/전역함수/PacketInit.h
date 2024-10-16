#ifndef  __LOBBYSERVER_PACKETINIT_H__
#define __LOBBYSERVER_PACKETINIT_H__
#include <Windows.h>
#include <string>
#include <vector>
using std::string;
using std::vector;

#include "공용 라이브러리/SerialRingBuffer.h"
#include "StoreManager.h"
#include "ItemManager.h"
#include "Inventory.h"

#include "ServerStatus.h"
#include "Friend.h"
#include "CharacterSeq.h"
#include "LobbyProtocol.h"
#include "en_LobbyServer.h"

class st_User;

void Packet_Init_Login(CPacket& packet, const vector<st_CharacterSeq>& CharacterSeqList);

void Packet_Init_SelectCharacter(CPacket& packet, const st_User* user);

void Packet_Init_SelectCharacter_Item(CPacket& packet, const st_User* user);

void Packet_Init_Character_List(CPacket& packet, const vector<st_CharacterSeq>& CharacterSeqList);

void Packet_Init_AvailableNickname(CPacket& packet, const BYTE& Status);

void Packet_Init_Create_Character(CPacket& packet, const BYTE& Status);

//------------------------------------------------------------------------------------------------------

void Packet_Init_Store_List(CPacket& packet, const vector<st_Product>& products);

void Packet_Init_Buy_Store_Item(CPacket& packet, const BYTE& status);
void Packet_Init_Buy_Store_Item(CPacket& packet, const BYTE& status, const BYTE& moneyID, const DWORD& moneyQuantity, const BYTE& tokenID, const st_InventoryUpdateInfo& tokenSlotUpdateInfo, const st_Item& item, const st_InventoryUpdateInfo& itemSlotUpdateInfo);

void Packet_Init_Show_Me_The_Money(CPacket& packet, const BYTE& moneyID, const DWORD& moneyQuantity);



// 아이템 사용 패킷
void Packet_Init_Use_Item_Failed(CPacket& packet, const BYTE& status);
void Packet_Init_Use_Item_StatsBox(CPacket& packet, const BYTE& statsID, const DWORD& stats, const st_Item& item, const st_InventoryUpdateInfo& inventoryUpdateInfo);

//------------------------------------------------------------------------------------------------------

void Packet_Init_Send_Whisper(CPacket& packet, const BYTE& status);

void Packet_Init_SC_Send_Whisper(CPacket& packet, const string& SendNickname, const string& Chatting);

void Packet_Init_Req_Friend(CPacket& packet, const string& SendNickname);

void Packet_Init_Accept_Friend(CPacket& packet, const string& SendNickname);

void Packet_Init_Move_Field(CPacket& packet, const BYTE& status, const DWORD& roomNo, const string& fieldName);

void Packet_Init_Move_Channel(CPacket& packet, const string& SessionKey);

void Packet_Init_Channel_List(CPacket& packet, const vector<st_ServerStatus>& ServerStatusList);

void Packet_Init_Add_Friend(CPacket& packet, const BYTE& status);

void Packet_Init_Accept_AddFriend(CPacket& packet, const BYTE& status, const string& AcceptedNickname);

void Packet_Init_Friend_List(CPacket& packet, const vector<st_Friend>& Friends);




#endif // __LOBBYSERVER_PACKETINIT_H__


