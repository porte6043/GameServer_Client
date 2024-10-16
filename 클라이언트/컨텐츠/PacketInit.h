#ifndef __PACKETINIT_H__
#define __PACKETINIT_H__
#include <string>
using std::string;

#include "공용 라이브러리/SerialRingBuffer.h"


namespace ns_LoginClient
{
	void Packet_Init_Login(CPacket& packet, string id, string password);

	void Packet_Init_Select_Server(CPacket& packet, DWORD64 uid);
}



namespace ns_GameClient
{
	void Packet_Init_Login(CPacket& packet, DWORD64 uid, string token);

	void Packet_Init_Login_Other_Channel(CPacket& packet, DWORD64 uid, string token, string nickname);

	void Packet_Init_Select_Character(CPacket& packet, string characterName);

	void Packet_Init_Create_Character(CPacket& packet, string characterName);

	void Packet_Init_CharacterList(CPacket& packet);

	void Packet_Init_Available_Nickname(CPacket& packet, string characterName);

	void Packet_Init_Ready_Field(CPacket& packet);

	void Packet_Init_Move_Field(CPacket& packet, DWORD roomNo);

	void Packet_Init_Move_Channel(CPacket& packet, string ip, WORD port);

	void Packet_Init_Channel_List(CPacket& packet);

	void Packet_Init_Store_Product_List(CPacket& packet, BYTE storeID);

	void Packet_Init_Buy_Store_Item(CPacket& packet, BYTE storeID, DWORD itemID, DWORD quantity);

	void Packet_Init_Show_Me_The_Money(CPacket& packet, BYTE moneyID);

	void Packet_Init_Use_Item(CPacket& packet, DWORD itemID);

	void Packet_Init_Add_Friend(CPacket& packet, string nickname);

	void Packet_Init_Accept_Add_Friend(CPacket& packet, string nickname);

	void Packet_Init_Friend_List(CPacket& packet);

	void Packet_Init_Send_Whishper(CPacket& packet, string recvNickname, string chatting);

}


#endif
