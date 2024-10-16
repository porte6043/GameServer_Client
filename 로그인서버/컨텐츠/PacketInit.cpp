#include "PacketInit.h"
#include "LoginProtocol.h"
#include "StringUtilities.h"



void Packet_Init_Login(CPacket& packet, BYTE status, DWORD64 uid, const std::vector<st_HashEntry>& serverStatus)
{
	packet << en_PACKET_SC_LOGIN_RES_LOGIN << status << uid << serverStatus.size();
	for (auto& field_value : serverStatus)
	{
		auto server = SplitString(field_value.field, ':');	// { ip, port, serverName }
		packet.PutData(server[0], 16);
		packet << static_cast<WORD>(stoi(server[1]));
		packet.PutData(server[2], 20);
		packet << static_cast<BYTE>(stoi(field_value.value));
	}
}

void Packet_Init_Select_Server(CPacket& packet, const string& token)
{
	packet << en_PACKET_SC_LOGIN_RES_SELECT_SERVER;
	packet.PutData(token, 64);
}