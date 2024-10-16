#ifndef __WAN_NETWOKR_CLIENT__
#define __WAN_NETWOKR_CLIENT__
#define IOCOUNT(Flag, IOCount) ( ((DWORD)Flag << 16) |  (DWORD)IOCount )
#define IOCOUNT_FLAG_HIGH(IOCount) (DWORD)(IOCount >> 16)
#define IOCOUNT_IOCOUNT_LOW(IOCount) (DWORD)(IOCount & 0x0000'ffff)

#define df_PACKET_CODE 0x77


#pragma comment(lib, "ws2_32") // WinSock2.h 라이브러리
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <process.h>
#include <unordered_map>
#include <queue>
#include <stack>
#include <list>
#include <string>
#include <Windows.h>
using std::unordered_map;
using std::queue;
using std::stack;
using std::list;
using std::string;


#define PROFILE
#include "공용 라이브러리/TlsProfiling.h"
#include "공용 라이브러리/SerialRingBuffer.h"
#include "공용 라이브러리/PerformanceDataHelp.h"
#include "공용 라이브러리/CpuUsage.h"
#include "공용 라이브러리/TextPasing.h"
#include "공용 라이브러리/Log.h"
#include "공용 라이브러리/Monitor.h"
struct PerformanceNetworkData;

class CWanClient
{
private:
	struct Session
	{
		SOCKET socket;
		SerialRingBuffer RecvQ;
		SerialRingBuffer SendQ;
		OVERLAPPED RecvOverlapped;
		OVERLAPPED SendOverlapped;
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
			IOCount = 1;
			SendBufferCount = 0;
			ZeroMemory(&RecvOverlapped, sizeof(RecvOverlapped));
			ZeroMemory(&SendOverlapped, sizeof(SendOverlapped));
			InitializeCriticalSection(&CS_SendQ);
		}
	};
	struct Timer
	{
		void (*TimerFunction)(LPVOID);
		LARGE_INTEGER Time;
		LPVOID p;
	};
	struct PerformanceNetworkData
	{
		DWORD PacketTotalCount;		// SerialRingBuffer::GetPoolTotalCount();
		DWORD PacketUseCount;		// SerialRingBuffer::GetPoolUseCount();
		DWORD SendTPS;				//
		DWORD RecvTPS;				//

		PerformanceNetworkData() :
			PacketTotalCount(0),
			PacketUseCount(0),
			SendTPS(0),
			RecvTPS(0) {}
	};
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

private:
	Session _Session;
	HANDLE hTimers[MAXIMUM_WAIT_OBJECTS];
	Timer TimerList[MAXIMUM_WAIT_OBJECTS];
	long TimerCount;

	HANDLE hCP;					// IOCP 핸들
	HANDLE hTimer_Threads;		// Timer ID
	HANDLE* hWorker_Threads;	// 워커스레드 ID
	PerformanceNetworkData Pnd;	// 모니터용 변수
	CPerformanceDataHelp Phd;	// 모니터용 변수 (하드웨어)

	// Systme
	WCHAR IP[16];				// Server IP
	WORD Port;					// Server Port
	bool Nagle;					// Client Nagle 옵션
	bool ZeroCopySend;			// ZeroCopySend 옵션
	bool AutoConnect;			// SessionRelease 후 자동 connect 옵션
	BYTE _PacketCode;			// network 헤더 코드
	BYTE _PacketKey;			// network 헤더 코드
	WORD NumberOfCore;			// WorkerThread의 수




public:		CWanClient();
public:		~CWanClient();

	  // 서버와의 연결 성공 후 (connect 직후)
protected:	virtual void OnConnectServer() = 0;

		 // 서버와 연결이 끊어졌을 때
protected:	virtual void OnDisconnectServer() = 0;

		 // Message 수신 완료 후 Contents Packet 처리를 합니다.
protected:	virtual void OnRecv(CPacket& packet) = 0;


public:		bool Connect(const wchar_t* ip, WORD port, WORD numberOfCreateThreads, WORD numberOfConcurrentThreads, bool nagle, bool zeroCopySend, bool autoConnect, BYTE packetCode, BYTE packetKey);

			// 꼭 OnDisconnectServer()가 호출 된 후에 사용하자
protected:	bool ReConnect();	
			// 꼭 OnDisconnectServer()가 호출 된 후에 사용하자 (변경되는 ip, port가 멀티 스레드에 안전하지 않습니다.) 
protected:	bool ReConnect(wstring ip, WORD port);
protected:	bool Disconnect();

public:	bool SendMSG(CPacket& packet);
public:	bool SendMSG_PQCS(CPacket& packet);


public:		void GetPND(PerformanceNetworkData& pnd);
public:		void GetPHD(PerformanceHardwareData& phd);

protected:	void RegisterTimer(void (*TimerFunction)(LPVOID), LPVOID p, DWORD dwMilliseconds);
private:	void NetworkError(int Err, const WCHAR* FunctionName);
private:	void NetworkMessageProc();
private:	void RecvPost();
private:	void SendPost();
private:	void SessionRelease();

	   //VS 출력창에 출력합니다.
protected:	void OutputDebugStringF(LPCTSTR format, ...);

		 // 함수 내부에서 일부로 크러쉬를 냅니다. 발생 하면 코드의 결함입니다.
protected:	void Crash();

private:	static unsigned WINAPI WorkerThreadWrapping(LPVOID p);
private:	void WorkerThread();

private:	static unsigned WINAPI TimerThreadWrapping(LPVOID p);
private:	void TimerThread();
};
#endif