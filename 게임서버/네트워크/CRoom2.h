#ifndef __NETWORK_ROOM2_h__
#define __NETWORK_ROOM2_h__
#pragma comment(lib, "Winmm.lib") // timeBeginPeriod 라이브러리


#include "네트워크/CWanServerRoom2.h"
#include "네트워크/PqcsEvent.h"
#include "네트워크/Session.h"
#include "네트워크/NetworkDefine.h"
#include "네트워크/RoomMessage.h"
#include "공용 라이브러리/SerialRingBuffer.h"


class CRoom2
{
	friend class CWanServerRoom2;
	using RoomSessionMap	= unordered_map<DWORD64, Session*>;
	using RoomQueue			= queue<Job_RoomMessageQ>;

private:
	RoomSessionMap			SessionMap;				// 현재 Room에 입장 되어 있는 세션
	RoomQueue				RoomMessageQ;				// Room
	RoomQueue				RoomWorkingQ;
	RoomQueue*				MessageQ;				// 외부에서 Enqueue 하는 Queue
	RoomQueue*				WorkingQ;				// Room에서 Dequeue 하는 Queue
	CRITICAL_SECTION		CS_RoomMessageQ;		// RoomQueue Lock
	CRITICAL_SECTION		CS_Room;				// Room Lock


	bool CalledChangeRoom;

private:
	CWanServerRoom2*	Network;
	DWORD				RoomNo;
	int		Frame;
	int		FrameTime;
	long	CurrentFrame;





public:		CRoom2();
public:		virtual ~CRoom2();

			// Room의 Network를 설정합니다.
protected:	virtual void OnSetNetwork(CWanServerRoom2* network) = 0;
			// Message 수신 완료 후 Contents Packet 처리를 합니다.
protected:	virtual void OnRecv(DWORD64 SessionID, CPacket& packet) = 0;
			// Session이 Room에 입장 된 후 호출됩니다
protected:	virtual void OnEnterRoom(DWORD64 SessionID, Param p) = 0;
			// Session이 Room에 퇴장 된 후 호출됩니다
protected:	virtual void OnLeaveRoom(DWORD64 SessionID) = 0;
			// Room이 삭제 될 때 남아 있는 Session의 처리를 위해 호출됩니다.(입장 요청 중인 Session 포함)
protected:	virtual void OnSessionKickOut(DWORD64 SessionID) = 0;
			// Session이 삭제 된 후 호출합니다. (연결이 끊어진 경우)
protected:	virtual void OnSessionRelease(DWORD64 SessionID) = 0;
			// RoomMsg를 처리합니다.
protected:	virtual void OnRecvToRoom(Param p) = 0;
			// Contents를 처리 합니다.
protected:	virtual void OnUpdate() = 0;

public:		void SendMSG(DWORD64 SessionID, CPacket& packet);
public:		void SendMSG_Disconnect(DWORD64 SessionID, CPacket& packet, DWORD dwSeconds);
public:		void Disconnect(DWORD64 SessionID);
			// 해당 함수 이후 SessionID의 유저의 Recv처리를 멈춥니다.
public:		void SuspendRecv(DWORD64 SessionID);
public:		bool ChangeRoom(DWORD64 SessionID, DWORD RoomNo, Param p = {});


public:		void SetFrame(int frame);
public:		long GetFrame();
public:		DWORD GetRoomNo();

private:	void QueueSwap();
private:	void RoomUpdate();
private:	void SetNetwork(CWanServerRoom2* network);

protected:	void Crash();
};
#endif