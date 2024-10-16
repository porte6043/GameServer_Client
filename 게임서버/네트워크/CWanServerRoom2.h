#ifndef __WAN_NETWORK_SERVER_ROOM_ARRANGE__
#define __WAN_NETWORK_SERVER_ROOM_ARRANGE__
#define PROFILE


#pragma comment(lib, "ws2_32") // WinSock2.h 라이브러리
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


#include "네트워크/PqcsEvent.h"
#include "네트워크/Session.h"
#include "네트워크/NetworkDefine.h"
#include "네트워크/RoomMessage.h"
#include "네트워크/CRoom2.h"
#include "공용 라이브러리/TlsProfiling.h"
#include "공용 라이브러리/SerialRingBuffer.h"
#include "공용 라이브러리/PerformanceDataHelp.h"
#include "공용 라이브러리/CpuUsage.h"
#include "공용 라이브러리/TextPasing.h"
#include "공용 라이브러리/Log.h"
#include "공용 라이브러리/Monitor.h"
#include "공용 라이브러리/LockFreeQueue.h"


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
	SOCKET ListenSocket;		// 리슨 소켓 ( AcceptThread를 종료할 떄 사용합니다.)
	HANDLE hAccept_Threads;		// Accept ID

	HANDLE hCP;					// IOCP 핸들
	vector<HANDLE> hWorker_Threads;	// 워커스레드 ID
	//HANDLE* hWorker_Threads;	// 워커스레드 ID

	HANDLE hTimer_Threads;		// Timer ID
	HANDLE	hTimers[MAXIMUM_WAIT_OBJECTS];
	//Timer	TimerList[MAXIMUM_WAIT_OBJECTS];
	function<void(void)> TimerFunctionList[MAXIMUM_WAIT_OBJECTS];
	long	TimerCount;
	bool	TimerEndFlag;


	PerformanceNetworkData Pnd;	// 모니터용 변수
	CPerformanceDataHelp Phd;	// 모니터용 변수 (하드웨어)


public:		CWanServerRoom2();

			 // 화이트 IP와 Port 접속 여부를 판단합니다. (Accpet 후)
private:	virtual bool OnAcceptRequest(WCHAR* IP, WORD Port) = 0;


public:		bool Start(const WCHAR* IP, WORD Port, WORD NumberOfCreateThreads, WORD NumberOfConcurrentThreads, bool Nagle, bool ZeroCopySend, BYTE PacketCode, BYTE PacketKey, WORD MaxSessionCount);
public:		bool Start(ServerSetting serverSetting);

			// 네트워크를 종료합니다.
public:		bool End();

protected:	bool Disconnect(DWORD64 SessionID);
private:	void Disconnect(Session* pSession);

protected:	bool SendMSG(DWORD64 SessionID, SerialRingBuffer* packet);
protected:	bool SendMSG(DWORD64 SessionID, CPacket& packet);
protected:	bool SendMSG_PQCS(DWORD64 SessionID, CPacket& packet);
protected:	bool SendMSG_PQCS_Disconnect(DWORD64 SessionID, CPacket& packet, DWORD dwSeconds);

			// SendMSG_PQCS_Disconnect 후에 time out 안에 Disconnect를 안 했을 경우 Disconnect를 합니다.
private:	void FlushDisconnectSession();

public:		void GetPND(PerformanceNetworkData& pnd);

public:		void GetPHD(PerformanceHardwareData& phd);

protected:	void RegisterTimer(std::function<void(void)> function, DWORD dwMilliseconds);

private:	bool ListenSocketInit(const WCHAR* IP, WORD Port, bool Nagle);

			// 세션의 IP와 Port를 얻어옵니다.
private:	void ClientInfo(Session* pSession, WCHAR* IP, WORD* Port);
private:	void ClientInfo(DWORD64 SessionID, WCHAR* IP, WORD* Port);

			// 네트워크의 에러가 발생할 시 로깅을 위한 함수입니다.
private:	void NetworkError(int Err, const WCHAR* FunctionName);

			// 네트워크 메시지의 헤더 확인해서 컨텐츠 Paload 추출 및 전달합니다.
private:	void NetworkMessageProc(Session* pSession);

			// WSARecv 보내기
private:	void RecvPost(Session* pSession);

			//  WSASend 보내기
private:	void SendPost(Session* pSession);

			// Session 삭제
private:	void SessionRelease(Session* pSession);

public	:	Session* SessionFind(DWORD64 SessionID);

			// 새로운 세션 할당을 위해서 사용 중이 않는 세션을 찾습니다.
private:	Session* SessionAlloc(SOCKET Socket, DWORD64 sessionID);

			// 해제된 세션ID을 반환합니다.
private:	DWORD64 SessionFree(Session* pSession);

private:	Session* SessionFindAndLock(DWORD64 SessionID);

			// 내부에서 SessionRelease가 호출 되었으면 true, 아니면 false
private:	void SessionUnLock(Session* pSession);
private:	void SessionUnLock(DWORD64 SessionID);

			// 반환값이 false일 경우 스레드 종료를 의미합니다.
private:	bool PqcsProc(PqcsEvent, ULONG_PTR);



// Room 함수
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
			// 동일한 RoomNo가 존재
			ReleaseSRWLockExclusive(&SRW_RoomMap);
			return INVALID_ROOMNO;
		}
		RoomMap.insert(std::make_pair(RoomNo, NewRoom));
	}
	ReleaseSRWLockExclusive(&SRW_RoomMap);


	// RoomUpdatePQueue에 push
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
			* Session 관련 데이터를 보냈을 때 해당 Room에 Session이
			* 없을 경우 Session이 처리를 못하고 사라질 수 있습니다.
			*/
public:		bool SendToRoom(DWORD RoomNo, Param param);

public:		bool SendToRoomSession(DWORD64 SessionID, Param param);

private:	void FlushRoom();



			 // 출력창에 출력합니다.
public:	void OutputDebugStringF(LPCTSTR format, ...);

			// 함수 내부에서 일부로 크러쉬를 냅니다. 발생 하면 코드의 결함입니다.
protected:	void Crash();

private:	static unsigned WINAPI WorkerThreadWrapping(LPVOID p);
private:	void WorkerThread();

private:	static unsigned WINAPI TimerThreadWrapping(LPVOID p);
private:	void TimerThread();

private:	static unsigned WINAPI AcceptThreadWrapping(LPVOID p);
private:	 void AcceptThread();
};
#endif