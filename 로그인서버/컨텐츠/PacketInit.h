#ifndef __LOGINSERVER_PacketInit_H__
#define __LOGINSERVER_PacketInit_H__
#include <Windows.h>
#include <vector>
#include "공용 라이브러리/SerialRingBuffer.h"
#include "CRedis.h"

void Packet_Init_Login(CPacket& packet, BYTE status, DWORD64 uid, const std::vector<st_HashEntry>& serverStatus);

void Packet_Init_Select_Server(CPacket& packet, const string& token);

#endif
