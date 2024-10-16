#ifndef __WAN_NETWORK_SERVER__
#define __WAN_NETWORK_SERVER__
#define SESSIONID(SessionID, SessionIndex) ( ((DWORD64)SessionID << 32) | (DWORD64)SessionIndex )
#define SESSIONID_ID_HIGH(SessionID) (DWORD)(SessionID >> 32)
#define SESSIONID_INDEX_LOW(SessionID) (DWORD)(SessionID & 0x0000'0000'ffff'ffff)

#define IOCOUNT(Flag, IOCount) ( ((DWORD)Flag << 16) |  (DWORD)IOCount )
#define IOCOUNT_FLAG_HIGH(IOCount) (DWORD)(IOCount >> 16)
#define IOCOUNT_IOCOUNT_LOW(IOCount) (DWORD)(IOCount & 0x0000'ffff)

#define INVALID_SESSIONID 0


#pragma comment(lib, "ws2_32") // WinSock2.h ���̺귯��
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <process.h>
#include <unordered_map>
#include <queue>
#include <stack>
#include <list>
#include <Windows.h>
using namespace std;
using std::unordered_map;
using std::queue;
using std::stack;
using std::list;

#define PROFILE
#include "���� ���̺귯��/TlsProfiling.h"
#include "���� ���̺귯��/SerialRingBuffer.h"
#include "���� ���̺귯��/PerformanceDataHelp.h"
#include "���� ���̺귯��/CpuUsage.h"
#include "���� ���̺귯��/TextPasing.h"
#include "���� ���̺귯��/Log.h"
#include "���� ���̺귯��/Monitor.h"



class CWanServer
{
private:
	struct Session
	{
		SOCKET socket;
		SerialRingBuffer RecvQ;
		SerialRingBuffer SendQ;
		OVERLAPPED RecvOverlapped;
		OVERLAPPED SendOverlapped;
		DWORD64 SessionID;		//  High 4Byte : SessionID , Low 4Byte : Sessions Index
		DWORD SendFlag;			// '0': Not Sending , '1': Sending
		DWORD DisconnectFlag;
		DWORD IOCount;
		DWORD SendBufferCount;
		CRITICAL_SECTION CS_SendQ;


		Session()
		{
			socket = INVALID_SOCKET;
			SendFlag = 0;
			DisconnectFlag = 0;
			SessionID = INVALID_SESSIONID;
			IOCount = 0;
			SendBufferCount = 0;
			ZeroMemory(&RecvOverlapped, sizeof(RecvOverlapped));
			ZeroMemory(&SendOverlapped, sizeof(SendOverlapped));
			InitializeCriticalSection(&CS_SendQ);
		}
	};
private:
	struct DisconnectedSession
	{
		DWORD64 SessionID;
		DWORD dwSeconds;
		DWORD64	Tick;

		DisconnectedSession()
		{
			SessionID = INVALID_SESSIONID;
			dwSeconds = 0;
			Tick = 0;
		}
	};
private:
	struct Timer
	{
		void (*TimerFunction)(LPVOID);
		LARGE_INTEGER Time;
		LPVOID p;
	};
protected:
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
#pragma pack(1)
private:
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
public:
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
	Session* Sessions;
	stack<DWORD> SessionIndex;
	CRITICAL_SECTION CS_SessionIndex;
	HANDLE hTimers[MAXIMUM_WAIT_OBJECTS];
	Timer TimerList[MAXIMUM_WAIT_OBJECTS];
	long TimerCount;

	DisconnectedSession* DisconnectedSessions;	// ������ ������ Session��

	// System
	WORD Port;					// Server Port
	WORD MaxSessionCount;		// �ִ� Session ��
	bool _Nagle;				// Nagle �ɼ�
	bool _ZeroCopySend;			// ZeroCopySend �ɼ�
	BYTE _PacketCode;			// network ��� �ڵ�

	SOCKET ListenSocket;		// ���� ���� ( AcceptThread�� ������ �� ����մϴ�.)
	HANDLE hCP;					// IOCP �ڵ�
	HANDLE hAccept_Threads;		// Accept ID
	HANDLE hTimer_Threads;		// Timer ID
	HANDLE* hWorker_Threads;	// ��Ŀ������ ID
	PerformanceNetworkData Pnd;	// ����Ϳ� ����
	CPerformanceDataHelp Phd;	// ����Ϳ� ���� (�ϵ����)



public:		CWanServer();
public:		~CWanServer();

			// ȭ��Ʈ IP�� Port ���� ���θ� �Ǵ��մϴ�. (Accpet ��)
private:	virtual bool OnAcceptRequest(WCHAR* IP, WORD Port) = 0;

			// Accept �� Session�� ������ �� �������� Client�� Session ������ �˸��ϴ�.
private:	 virtual void OnSessionConnected(DWORD64 SessionID) = 0;

			// Session�� ���� �� �� ȣ���մϴ�.
private:	virtual void OnSessionRelease(DWORD64 SessionID) = 0;

			// Message ���� �Ϸ� �� Contents Packet ó���� �մϴ�.
private:	virtual void OnRecv(DWORD64 SessionID, CPacket& packet) = 0;


public:		bool Start(const WCHAR* IP, WORD Port, WORD NumberOfCreateThreads, WORD NumberOfConcurrentThreads, bool Nagle, bool ZeroCopySend, BYTE PacketCode, BYTE PacketKey, WORD MaxSessionCount);
public:		bool Start(ServerSetting serverSetting);

protected:	bool Disconnect(DWORD64 SessionID);
private:	void Disconnect(Session* pSession);

protected:	bool SendMSG(DWORD64 SessionID, CPacket& packet);
protected:	bool SendMSG_PQCS(DWORD64 SessionID, CPacket& packet);
protected:	bool SendMSG_Disconnect(DWORD64 SessionID, CPacket& packet, DWORD dwSeconds);


public:		void GetPND(PerformanceNetworkData& pnd);
public:		void GetPHD(PerformanceHardwareData& phd);


protected:	void RegisterTimer(void (*TimerFunction)(LPVOID), LPVOID p, DWORD dwMilliseconds);

private:	bool ListenSocketInit(const WCHAR* IP, WORD Port, bool Nagle);

			// ������ IP�� Port�� ���ɴϴ�.
private:	void ClientInfo(Session* pSession, WCHAR* IP, WORD* Port);
protected:	bool ClientInfo(DWORD64 SessionID, WCHAR* IP, WORD* Port);

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

private:	Session* SessionFind(DWORD64 SessionID);

			// ���ο� ���� �Ҵ��� ���ؼ� ��� ���� �ʴ� ������ ã���ϴ�.
private:	Session* SessionAlloc(SOCKET Socket, DWORD64 sessionID);

			// ������ ����ID�� ��ȯ�մϴ�.
private:	DWORD64 SessionFree(Session* pSession);

			// ���â�� ����մϴ�.
protected:	void OutputDebugStringF(LPCTSTR format, ...);

			// �Լ� ���ο��� �Ϻη� ũ������ ���ϴ�. �߻� �ϸ� �ڵ��� �����Դϴ�.
protected:	void Crash();

			// ������ ������ Session�� �����մϴ�.
private:	static void DisconnectSession(LPVOID p);

private:	static unsigned WINAPI WorkerThreadWrapping(LPVOID p);
private:	void WorkerThread();

private:	static unsigned WINAPI TimerThreadWrapping(LPVOID p);
private:	void TimerThread();

private:	static unsigned WINAPI AcceptThreadWrapping(LPVOID p);
private:	 void AcceptThread();
};
#endif