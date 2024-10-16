#include "PacketInit.h"
#include "LoginProtocol.h"
#include "LobbyProtocol.h"

namespace ns_LoginClient
{
	void Packet_Init_Login(CPacket& packet, string id, string password)
	{
		packet << en_PACKET_CS_LOGIN_REQ_LOGIN;
		packet.PutData(id, 20);
		packet.PutData(password, 20);
	}

	void Packet_Init_Select_Server(CPacket& packet, DWORD64 uid)
	{
		packet << en_PACKET_CS_LOGIN_REQ_SELECT_SERVER << uid;
	}

}


namespace ns_GameClient
{
	void Packet_Init_Login(CPacket& packet, DWORD64 uid, string token)
	{
		packet << en_PACKET_CS_LOBBY_REQ_LOGIN << uid;
		packet.PutData(token, 64);
	}

	void Packet_Init_Login_Other_Channel(CPacket& packet, DWORD64 uid, string token, string nickname)
	{
		packet << en_PACKET_CS_LOBBY_REQ_LOGIN_OTHER_CHANNEL << uid;
		packet.PutData(token, 64);
		packet.PutData(nickname, 20);
	}

	void Packet_Init_Select_Character(CPacket& packet, string characterName)
	{
		packet << en_PACKET_CS_LOBBY_REQ_SELECT_CHARACTER;
		packet.PutData(characterName, 20);
	}

	void Packet_Init_Create_Character(CPacket& packet, string characterName)
	{
		packet << en_PACKET_CS_LOBBY_REQ_CREATE_CHARACTER;
		packet.PutData(characterName, 20);
	}

	void Packet_Init_CharacterList(CPacket& packet)
	{
		packet << en_PACKET_CS_LOBBY_REQ_CHARACTERLIST;
	}

	void Packet_Init_Available_Nickname(CPacket& packet, string characterName)
	{
		packet << en_PACKET_CS_LOBBY_REQ_AVAILABLE_NICKNAME;
		packet.PutData(characterName, 20);
	}

	void Packet_Init_Ready_Field(CPacket& packet)
	{
		packet << en_PACKET_CS_LOBBY_READY_FIELD;
	}

	void Packet_Init_Move_Field(CPacket& packet, DWORD roomNo)
	{
		packet << en_PACKET_CS_LOBBY_REQ_MOVE_FIELD << roomNo;
	}

	void Packet_Init_Move_Channel(CPacket& packet, string ip, WORD port)
	{
		packet << en_PACKET_CS_LOBBY_REQ_MOVE_CHANNEL;
		packet.PutData(ip, 16);
		packet << port;
	}

	void Packet_Init_Channel_List(CPacket& packet)
	{
		packet << en_PACKET_CS_LOBBY_REQ_CHANNEL_LIST;
	}

	void Packet_Init_Store_Product_List(CPacket& packet, BYTE storeID)
	{
		packet << en_PACKET_CS_LOBBY_REQ_STORE_PRODUCT_LIST << storeID;
	}

	void Packet_Init_Buy_Store_Item(CPacket& packet, BYTE storeID, DWORD itemID, DWORD quantity)
	{
		packet << en_PACKET_CS_LOBBY_REQ_BUY_STORE_ITEM << storeID << itemID << quantity;
	}

	void Packet_Init_Show_Me_The_Money(CPacket& packet, BYTE moneyID)
	{
		packet << en_PACKET_CS_LOBBY_REQ_SHOW_ME_THE_MONEY << moneyID;
	}

	void Packet_Init_Use_Item(CPacket& packet, DWORD itemID)
	{
		packet << en_PACKET_CS_LOBBY_REQ_USE_ITEM << itemID;
	}


	void Packet_Init_Add_Friend(CPacket& packet, string nickname)
	{
		packet << en_PACKET_CS_LOBBY_REQ_ADD_FRIEND;
		packet.PutData(nickname, 20);
	}

	void Packet_Init_Accept_Add_Friend(CPacket& packet, string nickname)
	{
		packet << en_PACKET_CS_LOBBY_REQ_ACCEPT_ADD_FRIEND;
		packet.PutData(nickname, 20);
	}

	void Packet_Init_Friend_List(CPacket& packet)
	{
		packet << en_PACKET_CS_LOBBY_REQ_FRIEND_LIST;
	}

	void Packet_Init_Send_Whishper(CPacket& packet, string recvNickname, string chatting)
	{
		packet << en_PACKET_CS_LOBBY_REQ_SEND_WHISPER;
		packet.PutData(recvNickname, 20);
		packet.PutData(chatting, 512);
	}
}