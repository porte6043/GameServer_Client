#ifndef __NETWORK_ROOM2_h__
#define __NETWORK_ROOM2_h__
#pragma comment(lib, "Winmm.lib") // timeBeginPeriod ���̺귯��


#include "��Ʈ��ũ/CWanServerRoom2.h"
#include "��Ʈ��ũ/PqcsEvent.h"
#include "��Ʈ��ũ/Session.h"
#include "��Ʈ��ũ/NetworkDefine.h"
#include "��Ʈ��ũ/RoomMessage.h"
#include "���� ���̺귯��/SerialRingBuffer.h"


class CRoom2
{
	friend class CWanServerRoom2;
	using RoomSessionMap	= unordered_map<DWORD64, Session*>;
	using RoomQueue			= queue<Job_RoomMessageQ>;

private:
	RoomSessionMap			SessionMap;				// ���� Room�� ���� �Ǿ� �ִ� ����
	RoomQueue				RoomMessageQ;				// Room
	RoomQueue				RoomWorkingQ;
	RoomQueue*				MessageQ;				// �ܺο��� Enqueue �ϴ� Queue
	RoomQueue*				WorkingQ;				// Room���� Dequeue �ϴ� Queue
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

			// Room�� Network�� �����մϴ�.
protected:	virtual void OnSetNetwork(CWanServerRoom2* network) = 0;
			// Message ���� �Ϸ� �� Contents Packet ó���� �մϴ�.
protected:	virtual void OnRecv(DWORD64 SessionID, CPacket& packet) = 0;
			// Session�� Room�� ���� �� �� ȣ��˴ϴ�
protected:	virtual void OnEnterRoom(DWORD64 SessionID, Param p) = 0;
			// Session�� Room�� ���� �� �� ȣ��˴ϴ�
protected:	virtual void OnLeaveRoom(DWORD64 SessionID) = 0;
			// Room�� ���� �� �� ���� �ִ� Session�� ó���� ���� ȣ��˴ϴ�.(���� ��û ���� Session ����)
protected:	virtual void OnSessionKickOut(DWORD64 SessionID) = 0;
			// Session�� ���� �� �� ȣ���մϴ�. (������ ������ ���)
protected:	virtual void OnSessionRelease(DWORD64 SessionID) = 0;
			// RoomMsg�� ó���մϴ�.
protected:	virtual void OnRecvToRoom(Param p) = 0;
			// Contents�� ó�� �մϴ�.
protected:	virtual void OnUpdate() = 0;

public:		void SendMSG(DWORD64 SessionID, CPacket& packet);
public:		void SendMSG_Disconnect(DWORD64 SessionID, CPacket& packet, DWORD dwSeconds);
public:		void Disconnect(DWORD64 SessionID);
			// �ش� �Լ� ���� SessionID�� ������ Recvó���� ����ϴ�.
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