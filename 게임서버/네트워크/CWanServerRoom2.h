#ifndef __WAN_NETWORK_SERVER_ROOM_ARRANGE__
#define __WAN_NETWORK_SERVER_ROOM_ARRANGE__
#define PROFILE


#pragma comment(lib, "ws2_32") // WinSock2.h ���̺귯��
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <process.h>
#include <unordered_map>
#include <queue>
#include <vector>
#include <stack>
#include <list>
#include <functional>
#include <Windows.h>
using std::unordered_map;
using std::queue;
using std::vector;
using std::stack;
using std::list;
using std::function;
using std::priority_queue;


#include "��Ʈ��ũ/PqcsEvent.h"
#include "��Ʈ��ũ/Session.h"
#include "��Ʈ��ũ/NetworkDefine.h"
#include "��Ʈ��ũ/RoomMessage.h"
#include "��Ʈ��ũ/CRoom2.h"
#include "���� ���̺귯��/TlsProfiling.h"
#include "���� ���̺귯��/SerialRingBuffer.h"
#include "���� ���̺귯��/PerformanceDataHelp.h"
#include "���� ���̺귯��/CpuUsage.h"
#include "���� ���̺귯��/TextPasing.h"
#include "���� ���̺귯��/Log.h"
#include "���� ���̺귯��/Monitor.h"
#include "���� ���̺귯��/LockFreeQueue.h"


class CWanServerRoom2
{
	friend class CRoom2;

private:
	#pragma pack(1)
	struct NetworkHeader
	{
		BYTE Code;
		WORD Len;
		BYTE RandKey;
		BYTE CheckSum;
		NetworkHeader()
		{
			Code = 0;
			Len = 0;
			RandKey = 0;
			CheckSum = 0;
		}
		NetworkHeader(BYTE code, SerialRingBuffer* packet)
		{
			Code = code;
			Len = packet->GetUseSize();
			RandKey = rand() % 256;
			CheckSum = 0;

			char* byte = packet->front();
			for (int iCnt = 0; iCnt < Len; ++iCnt)
			{
				CheckSum += *(byte + iCnt);
			}
		}
	};
	#pragma pack()
	struct Timer
	{
		std::function<void(void)> TimerFunction;
		LARGE_INTEGER Time;
	};
protected:
	struct Job_PqTimer
	{
		LARGE_INTEGER	ExecuteTime;
		DWORD			RoomNo;

		bool operator>(const Job_PqTimer& other) const
		{
			return ExecuteTime.QuadPart > other.ExecuteTime.QuadPart;
		}
	};
	struct Job_DisconnectedSessionPQueue
	{
		ULONGLONG	ExecuteTime;
		DWORD64		SessionID;

		bool operator>(const Job_DisconnectedSessionPQueue& other) const
		{
			return ExecuteTime > other.ExecuteTime;
		}
	};
public:
	struct PerformanceNetworkData
	{
		DWORD SessionCount;			//
		DWORD AcceptTotalCount;		// 
		DWORD PacketTotalCount;		// SerialRingBuffer::GetPoolTotalCount();
		DWORD PacketUseCount;		// SerialRingBuffer::GetPoolUseCount();
		DWORD AcceptTPS;			//
		DWORD SendTPS;				//
		DWORD RecvTPS;				//

		PerformanceNetworkData() :
			SessionCount(0),
			AcceptTotalCount(0),
			PacketTotalCount(0),
			PacketUseCount(0),
			AcceptTPS(0),
			SendTPS(0),
			RecvTPS(0) {}
	};
	struct ServerSetting
	{
		char IP_char[16];
		WCHAR IP_wchar[16];
		WORD Port;
		WORD WorkerThread;
		WORD ActiveThread;
		bool Nagle;
		bool ZeroCopySend;
		WORD SessionMax;
		BYTE PacketCode;
		BYTE PacketKey;

		ServerSetting() :
			IP_char{},
			IP_wchar{},
			Port(0),
			WorkerThread(0),
			ActiveThread(0),
			Nagle(false),
			ZeroCopySend(false),
			SessionMax(0),
			PacketCode(0),
			PacketKey(0) {}
	};

private:
	using RoomUpdatePriorityQueue = std::priority_queue<Job_PqTimer, vector<Job_PqTimer>, std::greater<Job_PqTimer>>;
	using DisconnectedSessionPQueue = std::priority_queue<Job_DisconnectedSessionPQueue, vector<Job_DisconnectedSessionPQueue>, std::greater<Job_DisconnectedSessionPQueue>>;

private:
	Session*				Sessions;
	stack<DWORD>			SessionIndex;
	CRITICAL_SECTION		CS_SessionIndex;

	// Room
	unordered_map<DWORD, CRoom2*>	RoomMap;
	SRWLOCK							SRW_RoomMap;
	DWORD							BasicRoomNo;
	RoomUpdatePriorityQueue			RoomUpdatePQueue;
	CRITICAL_SECTION				CS_RoomUpdatePQueue;
	LARGE_INTEGER					Frequency;
	
	DisconnectedSessionPQueue		DisconnectUpdatePQueue;
	CRITICAL_SECTION				CS_DisconnectUpdatePQueue;


	ServerSetting	serverSetting;

	// HANDLE
	SOCKET ListenSocket;		// ���� ���� ( AcceptThread�� ������ �� ����մϴ�.)
	HANDLE hAccept_Threads;		// Accept ID

	HANDLE hCP;					// IOCP �ڵ�
	vector<HANDLE> hWorker_Threads;	// ��Ŀ������ ID
	//HANDLE* hWorker_Threads;	// ��Ŀ������ ID

	HANDLE hTimer_Threads;		// Timer ID
	HANDLE	hTimers[MAXIMUM_WAIT_OBJECTS];
	//Timer	TimerList[MAXIMUM_WAIT_OBJECTS];
	function<void(void)> TimerFunctionList[MAXIMUM_WAIT_OBJECTS];
	long	TimerCount;
	bool	TimerEndFlag;


	PerformanceNetworkData Pnd;	// ����Ϳ� ����
	CPerformanceDataHelp Phd;	// ����Ϳ� ���� (�ϵ����)


public:		CWanServerRoom2();

			 // ȭ��Ʈ IP�� Port ���� ���θ� �Ǵ��մϴ�. (Accpet ��)
private:	virtual bool OnAcceptRequest(WCHAR* IP, WORD Port) = 0;


public:		bool Start(const WCHAR* IP, WORD Port, WORD NumberOfCreateThreads, WORD NumberOfConcurrentThreads, bool Nagle, bool ZeroCopySend, BYTE PacketCode, BYTE PacketKey, WORD MaxSessionCount);
public:		bool Start(ServerSetting serverSetting);

			// ��Ʈ��ũ�� �����մϴ�.
public:		bool End();

protected:	bool Disconnect(DWORD64 SessionID);
private:	void Disconnect(Session* pSession);

protected:	bool SendMSG(DWORD64 SessionID, SerialRingBuffer* packet);
protected:	bool SendMSG(DWORD64 SessionID, CPacket& packet);
protected:	bool SendMSG_PQCS(DWORD64 SessionID, CPacket& packet);
protected:	bool SendMSG_PQCS_Disconnect(DWORD64 SessionID, CPacket& packet, DWORD dwSeconds);

			// SendMSG_PQCS_Disconnect �Ŀ� time out �ȿ� Disconnect�� �� ���� ��� Disconnect�� �մϴ�.
private:	void FlushDisconnectSession();

public:		void GetPND(PerformanceNetworkData& pnd);

public:		void GetPHD(PerformanceHardwareData& phd);

protected:	void RegisterTimer(std::function<void(void)> function, DWORD dwMilliseconds);

private:	bool ListenSocketInit(const WCHAR* IP, WORD Port, bool Nagle);

			// ������ IP�� Port�� ���ɴϴ�.
private:	void ClientInfo(Session* pSession, WCHAR* IP, WORD* Port);
private:	void ClientInfo(DWORD64 SessionID, WCHAR* IP, WORD* Port);

			// ��Ʈ��ũ�� ������ �߻��� �� �α��� ���� �Լ��Դϴ�.
private:	void NetworkError(int Err, const WCHAR* FunctionName);

			// ��Ʈ��ũ �޽����� ��� Ȯ���ؼ� ������ Paload ���� �� �����մϴ�.
private:	void NetworkMessageProc(Session* pSession);

			// WSARecv ������
private:	void RecvPost(Session* pSession);

			//  WSASend ������
private:	void SendPost(Session* pSession);

			// Session ����
private:	void SessionRelease(Session* pSession);

public	:	Session* SessionFind(DWORD64 SessionID);

			// ���ο� ���� �Ҵ��� ���ؼ� ��� ���� �ʴ� ������ ã���ϴ�.
private:	Session* SessionAlloc(SOCKET Socket, DWORD64 sessionID);

			// ������ ����ID�� ��ȯ�մϴ�.
private:	DWORD64 SessionFree(Session* pSession);

private:	Session* SessionFindAndLock(DWORD64 SessionID);

			// ���ο��� SessionRelease�� ȣ�� �Ǿ����� true, �ƴϸ� false
private:	void SessionUnLock(Session* pSession);
private:	void SessionUnLock(DWORD64 SessionID);

			// ��ȯ���� false�� ��� ������ ���Ḧ �ǹ��մϴ�.
private:	bool PqcsProc(PqcsEvent, ULONG_PTR);



// Room �Լ�
private:	void EnterRoom(DWORD64 SessionID, DWORD RoomNo, Param param);

protected:	void SetBasicRoom(DWORD RoomNo);

public:
	template<typename Room, typename std::enable_if<std::is_base_of<CRoom2, Room>::value>::type* = nullptr>
	DWORD CreateRoom(const DWORD& RoomNo)
{
	Room* NewRoom = new(std::nothrow) Room;
	if (NewRoom == NULL)
		return INVALID_ROOMNO;
	NewRoom->RoomNo = RoomNo;
	NewRoom->SetNetwork(this);

	AcquireSRWLockExclusive(&SRW_RoomMap);
	{
		auto iter = RoomMap.find(RoomNo);
		if (iter != RoomMap.end())
		{
			// ������ RoomNo�� ����
			ReleaseSRWLockExclusive(&SRW_RoomMap);
			return INVALID_ROOMNO;
		}
		RoomMap.insert(std::make_pair(RoomNo, NewRoom));
	}
	ReleaseSRWLockExclusive(&SRW_RoomMap);


	// RoomUpdatePQueue�� push
	EnterCriticalSection(&CS_RoomUpdatePQueue);
	LARGE_INTEGER NextUpdateTime; 
	QueryPerformanceCounter(&NextUpdateTime);
	RoomUpdatePQueue.push({ NextUpdateTime, RoomNo });
	LeaveCriticalSection(&CS_RoomUpdatePQueue);

	return RoomNo;
}

public:		void DeleteRoom(DWORD);
private:	void ExecuteDeleteRoom(DWORD);

public:		DWORD GetRoomNo(DWORD64 SessionID);

public:		void SendToAllRoom(Param param);
private:	void ExecuteSendToAllRoom(Param param);
			
			/*
			* Session ���� �����͸� ������ �� �ش� Room�� Session��
			* ���� ��� Session�� ó���� ���ϰ� ����� �� �ֽ��ϴ�.
			*/
public:		bool SendToRoom(DWORD RoomNo, Param param);

public:		bool SendToRoomSession(DWORD64 SessionID, Param param);

private:	void FlushRoom();



			 // ���â�� ����մϴ�.
public:	void OutputDebugStringF(LPCTSTR format, ...);

			// �Լ� ���ο��� �Ϻη� ũ������ ���ϴ�. �߻� �ϸ� �ڵ��� �����Դϴ�.
protected:	void Crash();

private:	static unsigned WINAPI WorkerThreadWrapping(LPVOID p);
private:	void WorkerThread();

private:	static unsigned WINAPI TimerThreadWrapping(LPVOID p);
private:	void TimerThread();

private:	static unsigned WINAPI AcceptThreadWrapping(LPVOID p);
private:	 void AcceptThread();
};
#endif