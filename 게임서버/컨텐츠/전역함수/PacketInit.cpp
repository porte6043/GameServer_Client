#include "PacketInit.h"

#include "Character.h"
#include "Money.h"



void Packet_Init_Login(CPacket& packet, const vector<st_CharacterSeq>& CharacterSeqList)
{
	packet << en_PACKET_SC_LOBBY_RES_LOGIN << CharacterSeqList.size();
	for (int idx = 0; idx < CharacterSeqList.size(); ++idx)
	{
		packet.PutData(CharacterSeqList[idx].nickname, LEN_NICKNAME);
		packet << CharacterSeqList[idx].seq;
	}
	return;
}

void Packet_Init_SelectCharacter(CPacket& packet, const st_User* user)
{
	packet << en_PACKET_SC_LOBBY_RES_SELECT_CHARACTER;
	packet.PutData(user->Nickname, LEN_NICKNAME);
	packet << user->uid << static_cast<DWORD>(user->roomNo);

	auto statsList = user->stats.getStatsList();
	packet << statsList.size();
	for (auto& stats : statsList)
	{
		packet << stats.id << stats.quantity;
		packet.PutData(stats.name, 20);
	}

	auto moneyList = user->inventory.getMoneyList();
	packet << moneyList.size();
	for (auto& money : moneyList)
	{
		packet << money.moneyID << money.quantity;
		packet.PutData(getMoneyName(money.moneyID), 20);
	}

	CItemManager itemManager;
	auto itemList = user->inventory.getItemList();
	packet << DWORD(128) << itemList.size();
	for (auto& itemInfo : itemList)
	{
		auto& item = itemManager.getItemByRef(itemInfo.itemID).value().get();
		packet << item.type << item.itemID;
		packet.PutData(item.name, LEN_ITEM_NAME);
		packet.PutData(item.description, LEN_ITEM_DISCRIPTION);
		packet << item.maxQuantity << itemInfo.seq << itemInfo.quantity;
	}

	packet << user->Friends.size();
	for (int idx = 0; idx < user->Friends.size(); ++idx)
	{
		packet.PutData(user->Friends[idx].nickname, LEN_NICKNAME);
	}

	packet << user->AddFriends.size();
	for (int idx = 0; idx < user->AddFriends.size(); ++idx)
	{
		packet.PutData(user->AddFriends[idx], LEN_NICKNAME);
	}
	return;
}

void Packet_Init_SelectCharacter_Item(CPacket& packet, const st_User* user)
{
	CItemManager itemManager;
	auto itemList = user->inventory.getItemList();
	packet << DWORD(128) << itemList.size();
	for (auto& itemInfo : itemList)
	{
		auto& item = itemManager.getItemByRef(itemInfo.itemID).value().get();
		packet << item.type << item.itemID;
		packet.PutData(item.name, LEN_ITEM_NAME);
		packet.PutData(item.description, LEN_ITEM_DISCRIPTION);
		packet << item.maxQuantity << itemInfo.seq << itemInfo.quantity;
	}
}

void Packet_Init_Character_List(CPacket& packet, const vector<st_CharacterSeq>& CharacterSeqList)
{
	packet << en_PACKET_SC_LOBBY_RES_CHARACTERLIST << CharacterSeqList.size();
	for (int idx = 0; idx < CharacterSeqList.size(); ++idx)
	{
		packet.PutData(CharacterSeqList[idx].nickname, LEN_NICKNAME);
		packet << CharacterSeqList[idx].seq;
	}
	return;
}

void Packet_Init_AvailableNickname(CPacket& packet, const BYTE& Status)
{
	packet << en_PACKET_SC_LOBBY_RES_AVAILABLE_NICKNAME << Status;
	return;
}

void Packet_Init_Create_Character(CPacket& packet, const BYTE& Status)
{
	packet << en_PACKET_SC_LOBBY_RES_CREATE_CHARACTER << Status;
	return;
}

//------------------------------------------------------------------------------------------------------

void Packet_Init_Store_List(CPacket& packet, const vector<st_Product>& products)
{
	CItemManager itemManager;

	packet << en_PACKET_SC_LOBBY_RES_STORE_PRODUCT_LIST << products.size();
	for (auto& product : products)
	{
		auto& item = itemManager.getItemByRef(product.itemID).value().get();

		packet << item.type << item.itemID;
		packet.PutData(item.name, LEN_ITEM_NAME);
		packet.PutData(item.description, LEN_ITEM_DISCRIPTION);

		packet << product.moneyID << product.moneyPrice << product.tokenID << product.tokenPrice;
	}
}

void Packet_Init_Buy_Store_Item(CPacket& packet, const BYTE& status)
{
	packet << en_PACKET_SC_LOBBY_RES_BUY_STORE_ITEM << status;
}
void Packet_Init_Buy_Store_Item(CPacket& packet, const BYTE& status, const BYTE& moneyID, const DWORD& moneyQuantity, const BYTE& tokenID, const st_InventoryUpdateInfo& tokenSlotUpdateInfo, const st_Item& item, const st_InventoryUpdateInfo& itemSlotUpdateInfo)
{
	packet << en_PACKET_SC_LOBBY_RES_BUY_STORE_ITEM << status << moneyID << moneyQuantity << tokenID << tokenSlotUpdateInfo.slotsUpdateInfo.size();
	for (int icnt = 0; icnt < tokenSlotUpdateInfo.slotsUpdateInfo.size(); ++icnt)
	{
		packet << tokenSlotUpdateInfo.slotsUpdateInfo[icnt].seq << tokenSlotUpdateInfo.slotsUpdateInfo[icnt].quantity;
	}
	
	packet << item.type << item.itemID;
	packet.PutData(item.name, LEN_ITEM_NAME);
	packet.PutData(item.description, LEN_ITEM_DISCRIPTION);
	packet << item.maxQuantity << itemSlotUpdateInfo.slotsUpdateInfo.size();
	for (int icnt = 0; icnt < itemSlotUpdateInfo.slotsUpdateInfo.size(); ++icnt)
	{
		packet << itemSlotUpdateInfo.slotsUpdateInfo[icnt].seq << itemSlotUpdateInfo.slotsUpdateInfo[icnt].quantity;
	}
}

void Packet_Init_Show_Me_The_Money(CPacket& packet, const BYTE& moneyID, const DWORD& moneyQuantity)
{
	packet << en_PACKET_SC_LOBBY_RES_SHOW_ME_THE_MONEY << moneyID << moneyQuantity;
}

// 아이템 사용 패킷
void Packet_Init_Use_Item_Failed(CPacket& packet, const BYTE& status)
{
	packet << en_PACKET_SC_LOBBY_RES_USE_ITEM_FAILED << status;
}
void Packet_Init_Use_Item_StatsBox(CPacket& packet, const BYTE& statsID, const DWORD& stats, const st_Item& item, const st_InventoryUpdateInfo& inventoryUpdateInfo)
{
	packet << en_PACKET_SC_LOBBY_RES_USE_ITEM_STATS_BOX << statsID << stats;

	packet << item.type << item.itemID;
	packet.PutData(item.name, LEN_ITEM_NAME);
	packet.PutData(item.description, LEN_ITEM_DISCRIPTION);
	packet << item.maxQuantity << inventoryUpdateInfo.slotsUpdateInfo.size();
	for (int idx = 0; idx < inventoryUpdateInfo.slotsUpdateInfo.size(); ++idx)
	{
		packet << inventoryUpdateInfo.slotsUpdateInfo[idx].seq << inventoryUpdateInfo.slotsUpdateInfo[idx].quantity;
	}
}


//------------------------------------------------------------------------------------------------------

void Packet_Init_Send_Whisper(CPacket& packet, const BYTE& status)
{
	packet << en_PACKET_SC_LOBBY_RES_SEND_WHISPER << status;
	return;
}

void Packet_Init_SC_Send_Whisper(CPacket& packet, const string& SendNickname, const string& Chatting)
{
	packet << en_PACKET_SC_LOBBY_REQ_SEND_WHISPER;
	packet.PutData(SendNickname, LEN_NICKNAME);
	packet.PutData(Chatting, LEN_CHATTHING);
	return;
}

void Packet_Init_Req_Friend(CPacket& packet, const string& SendNickname)
{
	packet << en_PACKET_SC_LOBBY_REQ_REQ_FRIEND;
	packet.PutData(SendNickname, LEN_NICKNAME);
	return;
}

void Packet_Init_Accept_Friend(CPacket& packet, const string& SendNickname)
{
	packet << en_PACKET_SC_LOBBY_REQ_ACCEPT_FRIEND;
	packet.PutData(SendNickname, LEN_NICKNAME);
	return;
}

void Packet_Init_Move_Field(CPacket& packet, const BYTE& status, const DWORD& roomNo, const string& fieldName)
{
	packet << en_PACKET_SC_LOBBY_RES_MOVE_FIELD << status << roomNo;
	packet.PutData(fieldName, 20);
	return;
}

void Packet_Init_Move_Channel(CPacket& packet, const string& SessionKey)
{
	packet << en_PACKET_SC_LOBBY_RES_MOVE_CHANNEL;
	packet.PutData(SessionKey, 64);
	return;
}

void Packet_Init_Channel_List(CPacket& packet, const vector<st_ServerStatus>& ServerStatusList)
{
	packet << en_PACKET_SC_LOBBY_RES_CHANNEL_LIST << ServerStatusList.size();
	for (int iCnt = 0; iCnt < ServerStatusList.size(); ++iCnt)
	{
		packet.PutData(ServerStatusList[iCnt].ServerName, 20);
		packet.PutData(ServerStatusList[iCnt].IP, 16);
		packet << ServerStatusList[iCnt].Port << ServerStatusList[iCnt].StatusType;
	}
	return;
}

void Packet_Init_Add_Friend(CPacket& packet, const BYTE& status)
{
	packet << en_PACKET_SC_LOBBY_RES_ADD_FRIEND << status;
	return;
}

void Packet_Init_Accept_AddFriend(CPacket& packet, const BYTE& status, const string& AcceptedNickname)
{
	packet << en_PACKET_SC_LOBBY_RES_ACCEPT_ADD_FRIEND << status;
	packet.PutData(AcceptedNickname, LEN_NICKNAME);
	return;
}

void Packet_Init_Friend_List(CPacket& packet, const vector<st_Friend>& Friends)
{
	packet << en_PACKET_SC_LOBBY_RES_FRIEND_LIST << Friends.size();
	for (auto& Friend : Friends)
	{
		packet.PutData(Friend.nickname.c_str(), LEN_NICKNAME);
		packet << Friend.isOnline;
	}
	return;
}

