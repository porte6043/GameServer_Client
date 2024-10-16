#ifndef __NETWORK_SESSION_H__
#define __NETWORK_SESSION_H__
#include <Windows.h>
#include <네트워크/NetworkDefine.h>
#include <공용 라이브러리/SerialRingBuffer.h>

typedef struct Session
{
	SOCKET				socket;
	SerialRingBuffer	RecvQ;
	SerialRingBuffer	SendQ;
	OVERLAPPED			RecvOverlapped;
	OVERLAPPED			SendOverlapped;
	DWORD64				SessionID;		//  High 4Byte : SessionID , Low 4Byte : Sessions Index
	DWORD				SendBufferCount;
	DWORD				RoomNo;
	SerialRingBuffer	MessageQ;
	CRITICAL_SECTION	CS_MessageQ;
	CRITICAL_SECTION	CS_SendQ;
	DWORD				SendDisconnectTime; // 초(s) 단위
	bool				SuspendRecv;
	DWORD SendFlag;			// '0': Not Sending , '1': Sending
	DWORD DisconnectFlag;
	DWORD IOCount;


	Session() : RecvQ(6400), SendQ(6400), MessageQ(6400)
		//Session() : RecvQ(6400), SendQ(6400)
	{
		socket = INVALID_SOCKET;
		SendFlag = 0;
		DisconnectFlag = 0;
		SessionID = INVALID_SESSIONID;
		IOCount = 0;
		SendBufferCount = 0;
		RoomNo = INVALID_ROOMNO;
		SendDisconnectTime = 0;
		SuspendRecv = false;
		ZeroMemory(&RecvOverlapped, sizeof(RecvOverlapped));
		ZeroMemory(&SendOverlapped, sizeof(SendOverlapped));
		InitializeCriticalSection(&CS_MessageQ);
		InitializeCriticalSection(&CS_SendQ);
	}
};

#endif