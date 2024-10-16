#include "CWanServerRoom2.h"

CWanServerRoom2::CWanServerRoom2(): 
	Sessions(nullptr),
	BasicRoomNo(INVALID_ROOMNO), 
	serverSetting(), 
	hTimers{}, 
	TimerFunctionList{}, 
	TimerCount(-1), 
	TimerEndFlag(true),
	ListenSocket(INVALID_SOCKET),
	hAccept_Threads(nullptr),
	hCP(nullptr), 
	hTimer_Threads(nullptr)
{

	QueryPerformanceFrequency(&Frequency);

	// CS 구조체 초기화
	InitializeCriticalSection(&CS_SessionIndex);
	InitializeCriticalSection(&CS_RoomUpdatePQueue);
	InitializeCriticalSection(&CS_DisconnectUpdatePQueue);
	InitializeSRWLock(&SRW_RoomMap);

	// RoomTimer 등록
	auto bindfunction = std::bind(&CWanServerRoom2::FlushRoom, this);
	RegisterTimer(bindfunction, 20);

	auto bindfunction2 = std::bind(&CWanServerRoom2::FlushDisconnectSession, this);
	RegisterTimer(bindfunction2, 1000);
}

bool CWanServerRoom2::Start(const WCHAR* IP, WORD Port, WORD NumberOfCreateThreads, WORD NumberOfConcurrentThreads, bool Nagle, bool ZeroCopySend, BYTE PacketCode, BYTE PacketKey, WORD MaxSessionCount)
{
	// wprintf의 한글 사용
	_wsetlocale(LC_ALL, L"korean");
	timeBeginPeriod(1);
	
	wmemcpy(serverSetting.IP_wchar, IP, 16);
	serverSetting.Port = Port;
	serverSetting.Nagle = Nagle;
	serverSetting.WorkerThread = NumberOfCreateThreads;
	serverSetting.ActiveThread = NumberOfConcurrentThreads;
	serverSetting.ZeroCopySend = ZeroCopySend;
	serverSetting.PacketCode = PacketCode;
	serverSetting.PacketKey = PacketKey;
	serverSetting.SessionMax = MaxSessionCount;

	// 네트워크 헤드 PacketKey Set
	SerialRingBuffer::SetPacketKey(PacketKey);

	//Session 생성
	Sessions = new(std::nothrow) Session[serverSetting.SessionMax];
	if (Sessions == NULL)
	{
		LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"Fail Session Alloc");
		return false;
	}

	// SessionArr의 비어있는 인덱스 넣기
	for (int iCnt = serverSetting.SessionMax - 1; iCnt >= 0; --iCnt)
	{
		SessionIndex.push(iCnt);
	}

	if (BasicRoomNo == INVALID_ROOMNO)
	{
		LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"Not Setting BasicRoomNo");
		return false;
	}

	// 입출력 완료 포트 생성
	hCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, serverSetting.ActiveThread);
	if (hCP == NULL)
	{
		int Err = GetLastError();
		LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"Fail CreateIoCompletionPort() ErrorCode : %d", Err);
		return false;
	}

	// IOCP 워커스레드 생성
	for (unsigned int iCnt = 0; iCnt < serverSetting.WorkerThread; ++iCnt)
	{
		HANDLE hThread = (HANDLE)_beginthreadex(nullptr, 0, WorkerThreadWrapping, this, 0, nullptr);
		if (hThread == 0)
		{
			int Err = errno;
			LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"Fail _beginthreadex() ErrorCode : %d", Err);
			return false;
		}
		hWorker_Threads.push_back(hThread);
	}

	// listen socket 초기화
	ListenSocketInit(IP, Port, Nagle);

	// Accept 스레드 생성
	hAccept_Threads = (HANDLE)_beginthreadex(nullptr, 0, AcceptThreadWrapping, this, 0, nullptr);
	if (hAccept_Threads == 0)
	{
		int Err = errno;
		LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"Fail _beginthreadex() ErrorCode : %d", Err);
		return false;
	}

	// 타이머 스레드 생성
	hTimer_Threads = (HANDLE)_beginthreadex(nullptr, 0, TimerThreadWrapping, this, 0, nullptr);
	if (hTimer_Threads == 0)
	{
		int Err = errno;
		LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"Fail _beginthreadex() ErrorCode : %d", Err);
		return false;
	}

	return true;
}

bool CWanServerRoom2::Start(ServerSetting _ServerSetting)
{
	// wprintf의 한글 사용
	_wsetlocale(LC_ALL, L"korean");
	timeBeginPeriod(1);

	serverSetting = _ServerSetting;

	// 네트워크 헤드 PacketKey Set
	SerialRingBuffer::SetPacketKey(serverSetting.PacketKey);

	//Session 생성
	Sessions = new(std::nothrow) Session[serverSetting.SessionMax];
	if (Sessions == NULL)
	{
		LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"Fail Session Alloc");
		return false;
	}

	// SessionArr의 비어있는 인덱스 넣기
	for (int iCnt = serverSetting.SessionMax - 1; iCnt >= 0; --iCnt)
	{
		SessionIndex.push(iCnt);
	}

	if (BasicRoomNo == INVALID_ROOMNO)
	{
		LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"Not Setting BasicRoomNo");
		return false;
	}

	// 입출력 완료 포트 생성
	hCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, serverSetting.ActiveThread);
	if (hCP == NULL)
	{
		int Err = GetLastError();
		LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"Fail CreateIoCompletionPort() ErrorCode : %d", Err);
		return false;
	}
	
	// IOCP 워커스레드 생성
	for (unsigned int iCnt = 0; iCnt < serverSetting.WorkerThread; ++iCnt)
	{
		HANDLE hThread = (HANDLE)_beginthreadex(nullptr, 0, WorkerThreadWrapping, this, 0, nullptr);
		if (hThread == 0)
		{
			int Err = errno;
			LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"Fail _beginthreadex() ErrorCode : %d", Err);
			return false;
		}
		hWorker_Threads.push_back(hThread);
	}

	// listen socket 초기화
	ListenSocketInit(serverSetting.IP_wchar, serverSetting.Port, serverSetting.Nagle);
	// Accept 스레드 생성
	hAccept_Threads = (HANDLE)_beginthreadex(nullptr, 0, AcceptThreadWrapping, this, 0, nullptr);
	if (hAccept_Threads == 0)
	{
		int Err = errno;
		LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"Fail _beginthreadex() ErrorCode : %d", Err);
		return false;
	}

	// 타이머 스레드 생성
	hTimer_Threads = (HANDLE)_beginthreadex(nullptr, 0, TimerThreadWrapping, this, 0, nullptr);
	if (hTimer_Threads == 0)
	{
		int Err = errno;
		LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"Fail _beginthreadex() ErrorCode : %d", Err);
		return false;
	}


	return true;
}

bool CWanServerRoom2::End()
{
	// accpet 스레드 종료
	closesocket(ListenSocket);
	if (WAIT_FAILED == WaitForSingleObject(hAccept_Threads, INFINITE))
	{
		int err = GetLastError();
		LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"Failed to close accept thread : ErrorCode : %d", err);
		Crash();
	}
	CloseHandle(hAccept_Threads);

	// session 종료 처리
	for (int idx = 0; idx < serverSetting.SessionMax; ++idx)
	{
		if(Sessions[idx].SessionID != INVALID_SESSIONID)
			Disconnect(Sessions[idx].SessionID);
	}
	while (SessionIndex.size() != serverSetting.SessionMax)
	{
		Sleep(1000);
	}

	// timer thread 종료
	TimerEndFlag = false;
	if (WAIT_FAILED == WaitForSingleObject(hTimer_Threads, INFINITE))
	{
		int err = GetLastError();
		LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"Failed to close timer thread : ErrorCode : %d", err);
		Crash();
	}
	CloseHandle(hTimer_Threads);
	for (int idx = 0; idx < TimerCount; ++idx)
	{
		CloseHandle(hTimers[idx]);
	}
	

	// worker 스레드 종료
	PostQueuedCompletionStatus(hCP, PQCS_TRANSFERRED, 0, reinterpret_cast<LPOVERLAPPED>(PqcsEvent::THREAD_DONE));
	if (WAIT_FAILED == WaitForMultipleObjects(hWorker_Threads.size(), hWorker_Threads.data(), true, INFINITE))
	{
		int err = GetLastError();
		LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"Failed to close worker thread : ErrorCode : %d", err);
		Crash();
	}
	CloseHandle(hCP);
	for (int idx = 0; idx < hWorker_Threads.size(); ++idx)
	{
		CloseHandle(hWorker_Threads[idx]);
	}

	// Room 정리
	for (auto& room : RoomMap)
	{
		CRoom2* pRoom = room.second;
		delete pRoom;
	}

	// session 정리
	delete[] Sessions;

	// lock 객체 삭제
	DeleteCriticalSection(&CS_SessionIndex);
	DeleteCriticalSection(&CS_RoomUpdatePQueue);
	DeleteCriticalSection(&CS_DisconnectUpdatePQueue);

	// 타이머 선명도 초기화
	timeEndPeriod(1);

	return 0;
}

bool CWanServerRoom2::Disconnect(DWORD64 SessionID)
{
	Session* pSession = SessionFind(SessionID);
	int retIOCount = InterlockedIncrement(&pSession->IOCount);

	if (IOCOUNT_FLAG_HIGH(retIOCount) == 1)
	{
		if (0 == InterlockedDecrement(&pSession->IOCount))
			SessionRelease(pSession);
		return false;
	}
	else
	{
		if (SessionID != pSession->SessionID)	// 검색한 후 SessionID에 해당하는 세션이 Release되어 다른 세션으로 사용된 경우
		{
			if (0 == InterlockedDecrement(&pSession->IOCount))
				SessionRelease(pSession);
			return false;
		}
	}

	InterlockedExchange(&pSession->DisconnectFlag, 1);
	CancelIoEx(reinterpret_cast<HANDLE>(pSession->socket), NULL);

	if (0 == InterlockedDecrement(&pSession->IOCount))
		SessionRelease(pSession);

	return true;
}

void CWanServerRoom2::Disconnect(Session* pSession)
{
	InterlockedExchange(&pSession->DisconnectFlag, 1);
	CancelIoEx(reinterpret_cast<HANDLE>(pSession->socket), NULL);

	return;
}

bool CWanServerRoom2::SendMSG(DWORD64 SessionID, SerialRingBuffer* packet)
{
	Session* pSession = SessionFind(SessionID);
	int retIOCount = InterlockedIncrement(&pSession->IOCount);
	if (IOCOUNT_FLAG_HIGH(retIOCount) == 1)
	{
		if (0 == InterlockedDecrement(&pSession->IOCount))
			SessionRelease(pSession);
		return false;
	}
	else
	{
		if (SessionID != pSession->SessionID)	// 검색한 후 SessionID에 해당하는 세션이 Release되어 다른 세션으로 사용된 경우
		{
			if (0 == InterlockedDecrement(&pSession->IOCount))
				SessionRelease(pSession);
			return false;
		}
	}

	if (pSession->DisconnectFlag == 1)
	{
		if (0 == InterlockedDecrement(&pSession->IOCount))
			SessionRelease(pSession);
		return false;
	}

	// SendQ의 버퍼 크기가 부족한 경우
	// 클라 측 TCP수신 버퍼가 꽉 차고 서버 측 TCP송신 버퍼가 전부 찬 상황 -> 클라 측에서 recv를 안하고 있는 경우
	if (sizeof(SerialRingBuffer*) > pSession->SendQ.GetFreeSize())
	{
		LOG(L"Network_WanServer", CLog::LEVEL_DEBUG, L"Lack Session SendQ buffer");
		Disconnect(pSession->SessionID);
		
		if (0 == InterlockedDecrement(&pSession->IOCount))
			SessionRelease(pSession);

		return false;
	}

	NetworkHeader NetworkHeader(serverSetting.PacketCode, packet);
	packet->SetWanHeader(&NetworkHeader, sizeof(NetworkHeader));
	packet->encode();

	packet->AddRef();
	EnterCriticalSection(&pSession->CS_SendQ);
	pSession->SendQ.enqueue(&packet, sizeof(SerialRingBuffer*));
	LeaveCriticalSection(&pSession->CS_SendQ);

	// WSASend
	SendPost(pSession);

	if (0 == InterlockedDecrement(&pSession->IOCount))
		SessionRelease(pSession);

	// Monitor용
	InterlockedIncrement(&Pnd.SendTPS);
	return true;
}

bool CWanServerRoom2::SendMSG(DWORD64 SessionID, CPacket& packet)
{
	Session* pSession = SessionFind(SessionID);
	int retIOCount = InterlockedIncrement(&pSession->IOCount);
	if (IOCOUNT_FLAG_HIGH(retIOCount) == 1)
	{
		if (0 == InterlockedDecrement(&pSession->IOCount))
			SessionRelease(pSession);
		return false;
	}
	else
	{
		if (SessionID != pSession->SessionID)	// 검색한 후 SessionID에 해당하는 세션이 Release되어 다른 세션으로 사용된 경우
		{
			if (0 == InterlockedDecrement(&pSession->IOCount))
				SessionRelease(pSession);
			return false;
		}
	}

	if (pSession->DisconnectFlag == 1)
	{
		if (0 == InterlockedDecrement(&pSession->IOCount))
			SessionRelease(pSession);
		return false;
	}

	// SendQ의 버퍼 크기가 부족한 경우
	// 클라 측 TCP수신 버퍼가 꽉 차고 서버 측 TCP송신 버퍼가 전부 찬 상황 -> 클라 측에서 recv를 안하고 있는 경우
	if (sizeof(SerialRingBuffer*) > pSession->SendQ.GetFreeSize())
	{
		LOG(L"Network_WanServer", CLog::LEVEL_DEBUG, L"Lack Session SendQ buffer");
		Disconnect(pSession->SessionID);

		if (0 == InterlockedDecrement(&pSession->IOCount))
			SessionRelease(pSession);
		return false;
	}

	SerialRingBuffer* Packet = packet.Packet;
	NetworkHeader NetworkHeader(serverSetting.PacketCode, Packet);
	Packet->SetWanHeader(&NetworkHeader, sizeof(NetworkHeader));
	Packet->encode();

	Packet->AddRef();
	EnterCriticalSection(&pSession->CS_SendQ);
	pSession->SendQ.enqueue(&Packet, sizeof(SerialRingBuffer*));
	LeaveCriticalSection(&pSession->CS_SendQ);

	// WSASend
	SendPost(pSession);

	if (0 == InterlockedDecrement(&pSession->IOCount))
		SessionRelease(pSession);

	// Monitor용
	InterlockedIncrement(&Pnd.SendTPS);
	return true;
}
// PQCS 버전
bool CWanServerRoom2::SendMSG_PQCS(DWORD64 SessionID, CPacket& packet)
{
	Session* pSession = SessionFind(SessionID);
	int retIOCount = InterlockedIncrement(&pSession->IOCount);
	if (IOCOUNT_FLAG_HIGH(retIOCount) == 1)
	{
		if (0 == InterlockedDecrement(&pSession->IOCount))
			SessionRelease(pSession);
		return false;
	}
	else
	{
		if (SessionID != pSession->SessionID)	// 검색한 후 SessionID에 해당하는 세션이 Release되어 다른 세션으로 사용된 경우
		{
			if (0 == InterlockedDecrement(&pSession->IOCount))
				SessionRelease(pSession);
			return false;
		}
	}

	if (pSession->DisconnectFlag == 1)
	{
		if (0 == InterlockedDecrement(&pSession->IOCount))
			SessionRelease(pSession);
		return false;
	}

	// SendQ의 버퍼 크기가 부족한 경우
	// 클라 측 TCP수신 버퍼가 꽉 차고 서버 측 TCP송신 버퍼가 전부 찬 상황 -> 클라 측에서 recv를 안하고 있는 경우
	if (sizeof(SerialRingBuffer*) > pSession->SendQ.GetFreeSize())
	{
		LOG(L"Network_WanServer", CLog::LEVEL_DEBUG, L"Lack Session SendQ buffer");
		Disconnect(pSession->SessionID);

		if (0 == InterlockedDecrement(&pSession->IOCount))
			SessionRelease(pSession);
		return false;
	}

	SerialRingBuffer* Packet = packet.Packet;
	NetworkHeader NetworkHeader(serverSetting.PacketCode, Packet);
	Packet->SetWanHeader(&NetworkHeader, sizeof(NetworkHeader));
	Packet->encode();

	Packet->AddRef();
	EnterCriticalSection(&pSession->CS_SendQ);
	pSession->SendQ.enqueue(&Packet, sizeof(SerialRingBuffer*));
	LeaveCriticalSection(&pSession->CS_SendQ);

	while (1)
	{
		if (1 == InterlockedExchange(&pSession->SendFlag, 1))
		{
			if (0 == InterlockedDecrement(&pSession->IOCount))
				SessionRelease(pSession);
			break;
		}

		// 내가 SendFlag를 가져왔다 (PQCS를 해야한다) 
		if (pSession->SendQ.GetUseSize() != 0)
		{
			InterlockedExchange(&pSession->SendFlag, 0);
			PostQueuedCompletionStatus(hCP, PQCS_TRANSFERRED, reinterpret_cast<ULONG_PTR>(pSession), reinterpret_cast<LPOVERLAPPED>(PqcsEvent::SEND_POST));
			break;
		}
		else
		{
			InterlockedExchange(&pSession->SendFlag, 0);
			if (pSession->SendQ.GetUseSize() != 0)
				continue;
			else
			{
				if (0 == InterlockedDecrement(&pSession->IOCount))
					SessionRelease(pSession);
				break;
			}
		}
	}


	// Monitor용
	InterlockedIncrement(&Pnd.SendTPS);
	return true;
}

bool CWanServerRoom2::SendMSG_PQCS_Disconnect(DWORD64 SessionID, CPacket& packet, DWORD dwSeconds)
{
	Session* pSession = SessionFind(SessionID);
	int retIOCount = InterlockedIncrement(&pSession->IOCount);
	if (IOCOUNT_FLAG_HIGH(retIOCount) == 1)
	{
		if (0 == InterlockedDecrement(&pSession->IOCount))
			SessionRelease(pSession);
		return false;
	}
	else
	{
		if (SessionID != pSession->SessionID)	// 검색한 후 SessionID에 해당하는 세션이 Release되어 다른 세션으로 사용된 경우
		{
			if (0 == InterlockedDecrement(&pSession->IOCount))
				SessionRelease(pSession);
			return false;
		}
	}

	if (pSession->DisconnectFlag == 1)
	{
		if (0 == InterlockedDecrement(&pSession->IOCount))
			SessionRelease(pSession);
		return false;
	}

	// SendQ의 버퍼 크기가 부족한 경우
	// 클라 측 TCP수신 버퍼가 꽉 차고 서버 측 TCP송신 버퍼가 전부 찬 상황 -> 클라 측에서 recv를 안하고 있는 경우
	if (sizeof(SerialRingBuffer*) > pSession->SendQ.GetFreeSize())
	{
		LOG(L"Network_WanServer", CLog::LEVEL_DEBUG, L"Lack Session SendQ buffer");
		Disconnect(pSession->SessionID);

		if (0 == InterlockedDecrement(&pSession->IOCount))
			SessionRelease(pSession);
		return false;
	}

	SerialRingBuffer* Packet = packet.Packet;
	NetworkHeader NetworkHeader(serverSetting.PacketCode, Packet);
	Packet->SetWanHeader(&NetworkHeader, sizeof(NetworkHeader));
	Packet->encode();

	Packet->AddRef();
	EnterCriticalSection(&pSession->CS_SendQ);
	pSession->SendQ.enqueue(&Packet, sizeof(SerialRingBuffer*));
	LeaveCriticalSection(&pSession->CS_SendQ);
	pSession->SendDisconnectTime = dwSeconds;	// Diconnect Time 설정

	PostQueuedCompletionStatus(hCP, PQCS_TRANSFERRED, reinterpret_cast<ULONG_PTR>(pSession), reinterpret_cast<LPOVERLAPPED>(PqcsEvent::SEND_POST_DISCONNECT));

	// Monitor용
	InterlockedIncrement(&Pnd.SendTPS);
	return true;
}

void CWanServerRoom2::FlushDisconnectSession()
{
	while (!DisconnectUpdatePQueue.empty())
	{
		ULONGLONG CurrentTime = GetTickCount64();
		EnterCriticalSection(&CS_DisconnectUpdatePQueue);
		auto& iter = DisconnectUpdatePQueue.top();
		if (CurrentTime >= iter.ExecuteTime)
			DisconnectUpdatePQueue.pop();
			
		LeaveCriticalSection(&CS_DisconnectUpdatePQueue);

		Disconnect(iter.SessionID);
	}

	return;
}


void CWanServerRoom2::GetPND(PerformanceNetworkData& pnd)
{
	pnd.SessionCount = serverSetting.SessionMax - SessionIndex.size();
	pnd.AcceptTotalCount = Pnd.AcceptTotalCount;
	pnd.PacketTotalCount = SerialRingBuffer::GetPoolTotalCount();
	pnd.PacketUseCount = SerialRingBuffer::GetPoolUseCount();
	pnd.AcceptTPS = InterlockedExchange(&Pnd.AcceptTPS, 0);
	pnd.SendTPS = InterlockedExchange(&Pnd.SendTPS, 0);
	pnd.RecvTPS = InterlockedExchange(&Pnd.RecvTPS, 0);
	return;
}

void CWanServerRoom2::GetPHD(PerformanceHardwareData& phd)
{
	Phd.GetPHD(phd);
	return;
}

bool CWanServerRoom2::ListenSocketInit(const WCHAR* IP, WORD Port, bool Nagle)
{
	// 윈속 초기화
	WSADATA WSA;
	if (WSAStartup(MAKEWORD(2, 2), &WSA) != 0)
	{
		int Err = WSAGetLastError();
		LOG(L"Network_WanServer", CLog::LEVEL_ERROR,
			L"Fail WSAStartup() ErrorCode : %d", Err);
		return false;
	}
	LOG(L"Network_WanServer", CLog::LEVEL_SYSTEM, L"WSAStartup()");


	// Listen 소켓 생성
	ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ListenSocket == INVALID_SOCKET)
	{
		int Err = WSAGetLastError();
		LOG(L"Network_WanServer", CLog::LEVEL_ERROR,
			L"Fail ListenSocket() ErrorCode : %d", Err);
		return false;
	}
	LOG(L"Network_WanServer", CLog::LEVEL_SYSTEM, L"ListenSocket()");

	//네이글 알고리즘 on/off
	int option = Nagle;
	setsockopt(ListenSocket,  // 해당 소켓
		IPPROTO_TCP,          // 소켓의 레벨
		TCP_NODELAY,          // 설정 옵션
		(const char*)&option, // 옵션 포인터
		sizeof(option));      // 옵션 크기

	// Listen 소켓 주소 초기화
	SOCKADDR_IN ListenSockAddr;
	ZeroMemory(&ListenSockAddr, sizeof(ListenSockAddr));
	ListenSockAddr.sin_family = AF_INET;
	InetPton(AF_INET, IP, &ListenSockAddr.sin_addr);
	ListenSockAddr.sin_port = htons(Port);

	// 바인딩 : std::bind랑 겹치기 떄문에 window의 bind는 전역이므로 이름없는 네임스페이스로 std::bind와 구분한다.
	if (::bind(ListenSocket, (sockaddr*)&ListenSockAddr, sizeof(ListenSockAddr)) == SOCKET_ERROR)	
	{
		int err = WSAGetLastError();
		LOG(L"Network_WanServer", CLog::LEVEL_ERROR,
			L"Fail bind() ErrorCode : %d", err);
		return false;
	}
	LOG(L"Network_WanServer", CLog::LEVEL_SYSTEM, L"bind() Port:%d", Port);

	// listen()
	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		LOG(L"Network_WanServer", CLog::LEVEL_ERROR,
			L"Fail listen() ErrorCode : %d", err);
		return false;
	}
	LOG(L"Network_WanServer", CLog::LEVEL_SYSTEM, L"listen()");

	return true;
}

void CWanServerRoom2::ClientInfo(Session* pSession, WCHAR* IP, WORD* Port)
{
	// 클라이언트 정보 얻기
	SOCKADDR_IN ClientAddr;
	int AddrLen = sizeof(ClientAddr);
	getpeername(pSession->socket, (SOCKADDR*)&ClientAddr, &AddrLen);

	// IP와 port 세팅
	*Port = ntohs(ClientAddr.sin_port);
	InetNtop(AF_INET, &ClientAddr.sin_addr, IP, 16);

	return;
}

void CWanServerRoom2::ClientInfo(DWORD64 SessionID, WCHAR* IP, WORD* Port)
{
	Session* pSession = SessionFind(SessionID);
	int retIOCount = InterlockedIncrement(&pSession->IOCount);

	if (IOCOUNT_FLAG_HIGH(retIOCount) == 1)
	{
		if (0 == InterlockedDecrement(&pSession->IOCount))
			SessionRelease(pSession);
		return;
	}
	else
	{
		if (SessionID != pSession->SessionID)	// 검색한 후 SessionID에 해당하는 세션이 Release되어 다른 세션으로 사용된 경우
		{
			if (0 == InterlockedDecrement(&pSession->IOCount))
				SessionRelease(pSession);
			return;
		}
	}

	// 클라이언트 정보 얻기
	SOCKADDR_IN ClientAddr;
	int AddrLen = sizeof(ClientAddr);
	getpeername(pSession->socket, (SOCKADDR*)&ClientAddr, &AddrLen);

	// IP와 port 세팅
	*Port = ntohs(ClientAddr.sin_port);
	InetNtop(AF_INET, &ClientAddr.sin_addr, IP, 16);

	if (0 == InterlockedDecrement(&pSession->IOCount))
		SessionRelease(pSession);
	return;
}

void CWanServerRoom2::NetworkError(int Err, const WCHAR* FunctionName)
{
	//Err == WSAEINVAL						// 10022 : WSASend에서 dwBufferCount 인수에 '0'이 들어갔을 떄 나타나는 것을 확인

	if (Err != WSAECONNRESET &&				// 10054 : 클라 측에서 끊었을 경우
		Err != WSAECONNABORTED &&			// 10053 : 소프트웨어(TCP)로 연결이 중단된 경우
		Err != ERROR_NETNAME_DELETED &&		// 64	 : RST, 클라에서 종료 했을 경우
		Err != WSAENOTSOCK &&				// 10038 : 소켓이 아닌 항목에서 작업을 시도했습니다.
		Err != ERROR_OPERATION_ABORTED &&	// 995	 : 스레드 종료 또는 애플리케이션 요청으로 인해 I/O 작업이 중단되었습니다. (CancelIoEx)
		Err != ERROR_SUCCESS)				// 0	 : 작업이 성공적으로 완료되었습니다. (FIN을 받았을 경우)
	{
		LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"Fail %s ErrorCode : %d", FunctionName, Err);
	}
	return;
}

void CWanServerRoom2::NetworkMessageProc(Session* pSession)
{
	while (1)
	{
		// NetworkHeader 보다 크게 있는지 확인
		if (sizeof(NetworkHeader) >= pSession->RecvQ.GetUseSize())
			break;

		// Network Header peek
		NetworkHeader NetworkHeader;
		int retpeek = pSession->RecvQ.peek(&NetworkHeader, sizeof(NetworkHeader));

		// code 확인
		if (NetworkHeader.Code != serverSetting.PacketCode)
		{
			LOG(L"Network_WanServer", CLog::LEVEL_DEBUG, L"Packet Error : Code Mismatch");
			Disconnect(pSession->SessionID);
			break;
		}

		// PayLoad가 버퍼 크기보다 큰지 확인
		if (NetworkHeader.Len > SerialRingBuffer::en_BUFFER_DEFAULT_SIZE - sizeof(NetworkHeader) - 1)
		{
			LOG(L"Network_WanServer", CLog::LEVEL_DEBUG, L"Packet Error : Buffer Size Overflow");
			Disconnect(pSession->SessionID);
			break;
		}

		// PayLoad의 크기가 0 인 경우
		if (NetworkHeader.Len == 0)
		{
			LOG(L"Network_WanServer", CLog::LEVEL_DEBUG, L"Packet Error : No PayLoad");
			Disconnect(pSession->SessionID);
			break;
		}

		// Session의 MessageQ의 버퍼 크기가 부족 할 경우
		if (sizeof(SerialRingBuffer*) > pSession->MessageQ.GetFreeSize())
		{
			LOG(L"Network_WanServer", CLog::LEVEL_DEBUG, L"Lack Session MessageQ buffer");
			Disconnect(pSession->SessionID);
			break;
		}


		// PayLoad가 들어왔는지 확인
		if (pSession->RecvQ.GetUseSize() - sizeof(NetworkHeader) >= NetworkHeader.Len)
		{
			// Network 헤더 크기만큼 이동
			pSession->RecvQ.MoveFront(sizeof(NetworkHeader));

			SerialRingBuffer* Packet = SerialRingBuffer::Alloc();
			Packet->SetWanHeader(&NetworkHeader, sizeof(NetworkHeader));
			int retDeq = pSession->RecvQ.dequeue(Packet->buffer(), NetworkHeader.Len);
			if (retDeq != NetworkHeader.Len)
			{
				LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"Failed dequeue()");
				Crash();
			}
			Packet->MoveRear(retDeq);

			// decode 실패
			if (!Packet->decode())
			{
				LOG(L"Network_WanServer", CLog::LEVEL_DEBUG, L"Failed decode()");
				Packet->Free();
				Disconnect(pSession->SessionID);
				break;
			}

			// SerialRingBuffer 방식
			Packet->AddRef();
			pSession->MessageQ.enqueue(&Packet, sizeof(SerialRingBuffer*));

			// Monitor 용
			InterlockedIncrement(&Pnd.RecvTPS);
		}
		else break;
	}

	return;
}

void CWanServerRoom2::RecvPost(Session* pSession)
{
	// 비동기 입출력 시작
	DWORD Flags = 0;
	WSABUF WsaBuf[2];
	WsaBuf[0].buf = pSession->RecvQ.rear();
	WsaBuf[0].len = pSession->RecvQ.DirectEnqueueSize();
	WsaBuf[1].buf = pSession->RecvQ.buffer();
	WsaBuf[1].len = pSession->RecvQ.GetFreeSize() - pSession->RecvQ.DirectEnqueueSize();
	ZeroMemory(&pSession->RecvOverlapped, sizeof(OVERLAPPED));

	InterlockedIncrement(&pSession->IOCount);
	int retRecv = WSARecv(pSession->socket, WsaBuf, 2, nullptr, &Flags, (OVERLAPPED*)&pSession->RecvOverlapped, NULL);
	if (retRecv == SOCKET_ERROR)
	{
		int Err = WSAGetLastError();
		if (Err != ERROR_IO_PENDING)
		{
			NetworkError(Err, L"WSARecv");
			// WSARecv를 실패 했을 경우 IOCount 감소
			if (0 == InterlockedDecrement(&pSession->IOCount))
				SessionRelease(pSession);

			// Socket의 연결이 끊어져서 이곳으로 들어오게 되면 Recv실패 완료 통지를 못 받는다 send쪽에서 실패 완료 통지가 떨어져서 Disconnect가 되어 DisconnectFlag가 올라가면 상관없는데 둘 다 아닐 경우
			// Room에 있는 Session은 DisconnectFlag를 받지 못해서 영영 사라지지않는다. 따라서 이 곳에서 DisconnectFlag를 올려준다.
			InterlockedExchange(&pSession->DisconnectFlag, 1);
		}
		else
		{
			if (pSession->DisconnectFlag == 1)
				CancelIoEx(reinterpret_cast<HANDLE>(pSession->socket), NULL);
		}
	}

	if (0 == InterlockedDecrement(&pSession->IOCount))
		SessionRelease(pSession);
	return;
}

void CWanServerRoom2::SendPost(Session* pSession)
{
	/*
	Send의 1회 제한
	1회로 제한 하지 않으면 WSASend 완료통지가 와서 링버퍼의 front가 처리 되기 전에 WSARecv 완료통지가
	먼저 와서 메시지 처리 후 WSARecv완료통지 쪽에서 다시 한번 WSASend를 하게 되면 이전 WSASend의 front가
	처리 되기 전에 Send를 하게 되어 메시지가 중첩된게 보내질 수 있다.
	*/
	SerialRingBuffer* SendQ = &pSession->SendQ;

	/*
		만약 지역변수로 UseSize를 사용하지 않으면, SendMSG로 인해 SendPost가 호출 되면 Session에 lock이 걸려있어서 상관 없지만
		WorkerThread에서 Send완료통지에서 SendPost를 호출 하는 경우 Session에 lock이 걸려있지 않기 때문에 모든 라인에서 GetUs
		esize()의 크기가 달라질 수 있다.
	*/
	while (1)
	{
		if (1 == InterlockedExchange(&pSession->SendFlag, 1))
			return;

		if (SendQ->GetUseSize() != 0)
		{
			break;
		}
		else
		{
			InterlockedExchange(&pSession->SendFlag, 0);\
			if (SendQ->GetUseSize() != 0)
				continue;
			else
				return;
		}
	}

	// SerialRingBuffer의 point를 SendQ에 넣습니다.
	WSABUF WsaBuf[800];
	pSession->SendBufferCount = SendQ->GetUseSize() / 8;
	unsigned int DirectSize = pSession->SendQ.DirectDequeueSize();
	if (DirectSize / 8 >= pSession->SendBufferCount)
	{
		for (unsigned int iCnt = 0; iCnt < pSession->SendBufferCount; ++iCnt)
		{
			SerialRingBuffer* pPacket = (SerialRingBuffer*)*((INT64*)(SendQ->front() + iCnt * 8));

			WsaBuf[iCnt].buf = pPacket->WanHead();
			WsaBuf[iCnt].len = pPacket->GetUseSize() + sizeof(NetworkHeader);
		}
		ZeroMemory(&pSession->SendOverlapped, sizeof(OVERLAPPED));
	}
	else
	{
		int index = 0;
		int iCnt_1 = DirectSize / 8;
		int iCnt_2 = pSession->SendBufferCount - iCnt_1 - 1;

		for (int iCnt = 0; iCnt < iCnt_1; ++iCnt)
		{
			SerialRingBuffer* pPacket = (SerialRingBuffer*)*((INT64*)(SendQ->front() + iCnt * 8));
			WsaBuf[index].buf = pPacket->WanHead();
			WsaBuf[index].len = pPacket->GetUseSize() + sizeof(NetworkHeader);
			++index;
		}

		char Buffer[8];
		memcpy(Buffer, SendQ->front() + iCnt_1 * 8, DirectSize - iCnt_1 * 8);
		memcpy(Buffer + DirectSize - iCnt_1 * 8, SendQ->buffer(), 8 - (DirectSize - iCnt_1 * 8));
		SerialRingBuffer* pPacket = (SerialRingBuffer*)*((INT64*)Buffer);
		WsaBuf[index].buf = pPacket->WanHead();
		WsaBuf[index].len = pPacket->GetUseSize() + sizeof(NetworkHeader);
		++index;

		char* front = SendQ->buffer() + 8 - (DirectSize - iCnt_1 * 8);
		for (int iCnt = 0; iCnt < iCnt_2; ++iCnt)
		{
			SerialRingBuffer* pPacket = (SerialRingBuffer*)*((INT64*)(front + iCnt * 8));
			WsaBuf[index].buf = pPacket->WanHead();
			WsaBuf[index].len = pPacket->GetUseSize() + sizeof(NetworkHeader);
			++index;
		}

		ZeroMemory(&pSession->SendOverlapped, sizeof(OVERLAPPED));
	}

	// IOCount의 증가
	InterlockedIncrement(&pSession->IOCount);
	int retSend = WSASend(pSession->socket, WsaBuf, pSession->SendBufferCount, nullptr, 0, (OVERLAPPED*)&pSession->SendOverlapped, NULL);
	if (retSend == SOCKET_ERROR)
	{
		int Err = WSAGetLastError();
		if (Err != WSA_IO_PENDING)
		{
			NetworkError(Err, L"WSASend");

			if (0 == InterlockedDecrement(&pSession->IOCount))
				SessionRelease(pSession);
		}
	}

	return;
}

void CWanServerRoom2::SessionRelease(Session* pSession)
{
	DWORD CompareIOCount = IOCOUNT(0, 0);
	DWORD ExchangeIOCount = IOCOUNT(1, 0);
	DWORD retIOCount = InterlockedCompareExchange(&pSession->IOCount, ExchangeIOCount, CompareIOCount);
	if (CompareIOCount != retIOCount)
		return;

	DWORD64 SessionID = SessionFree(pSession);

	return;
}

Session* CWanServerRoom2::SessionFind(DWORD64 SessionID)
{
	return &Sessions[SESSIONID_INDEX_LOW(SessionID)];
}

Session* CWanServerRoom2::SessionAlloc(SOCKET Socket, DWORD64 sessionID)
{
	while (SessionIndex.empty())
	{
		Sleep(1000);
	}

	EnterCriticalSection(&CS_SessionIndex);
	DWORD Index = SessionIndex.top();
	SessionIndex.pop();
	LeaveCriticalSection(&CS_SessionIndex);


	Sessions[Index].socket = Socket;
	Sessions[Index].SessionID = SESSIONID(sessionID, Index);
	Sessions[Index].SendFlag = 0;
	Sessions[Index].DisconnectFlag = 0;
	Sessions[Index].SendBufferCount = 0;
	Sessions[Index].RecvQ.clear();
	Sessions[Index].SendQ.clear();
	Sessions[Index].RoomNo = INVALID_ROOMNO;
	Sessions[Index].SuspendRecv = false;
	ZeroMemory(&Sessions[Index].RecvOverlapped, sizeof(OVERLAPPED));
	ZeroMemory(&Sessions[Index].SendOverlapped, sizeof(OVERLAPPED));
	InterlockedIncrement(&Sessions[Index].IOCount);
	InterlockedAnd((long*)&Sessions[Index].IOCount, 0x0000'ffff);

	return &Sessions[Index];
}

DWORD64 CWanServerRoom2::SessionFree(Session* pSession)
{
	closesocket(pSession->socket);

	DWORD64 SessionID = pSession->SessionID;
	DWORD Index = SESSIONID_INDEX_LOW(pSession->SessionID);
	DWORD sessionID = SESSIONID_ID_HIGH(pSession->SessionID);
	pSession->SessionID = INVALID_SESSIONID;
	pSession->socket = INVALID_SOCKET;


	// SendQ의 SerialRingBuffer RefCount Sub 및 SendQ에서 지우기
	{
		int UseSize = pSession->SendQ.GetUseSize();
		unsigned int DirectSize = pSession->SendQ.DirectDequeueSize();
		int SubCount = UseSize / 8;
		if (DirectSize / 8 >= SubCount)
		{
			for (unsigned int iCnt = 0; iCnt < SubCount; ++iCnt)
			{
				SerialRingBuffer* pPacket = (SerialRingBuffer*)*((INT64*)(pSession->SendQ.front() + iCnt * 8));
				pPacket->SubRef();
			}
		}
		else
		{
			int iCnt_1 = DirectSize / 8;
			int iCnt_2 = SubCount - iCnt_1 - 1;

			for (int iCnt = 0; iCnt < iCnt_1; ++iCnt)
			{
				SerialRingBuffer* pPacket = (SerialRingBuffer*)*((INT64*)(pSession->SendQ.front() + iCnt * 8));
				pPacket->SubRef();
			}
			char Buffer[8];

			memcpy(Buffer, pSession->SendQ.front() + iCnt_1 * 8, DirectSize - iCnt_1 * 8);
			memcpy(Buffer + DirectSize - iCnt_1 * 8, pSession->SendQ.buffer(), 8 - (DirectSize - iCnt_1 * 8));
			SerialRingBuffer* pPacket = (SerialRingBuffer*)*((INT64*)Buffer);
			pPacket->SubRef();

			char* front = pSession->SendQ.buffer() + 8 - (DirectSize - iCnt_1 * 8);
			for (int iCnt = 0; iCnt < iCnt_2; ++iCnt)
			{
				SerialRingBuffer* pPacket = (SerialRingBuffer*)*((INT64*)(front + iCnt * 8));
				pPacket->SubRef();
			}
		}
	}

	// SerialRingBuffer 방식
	{
		DWORD Count = pSession->MessageQ.GetUseSize() / 8;
		for (int iCnt = 0; iCnt < Count; ++iCnt)
		{
			SerialRingBuffer* pPacket;
			pSession->MessageQ.dequeue(&pPacket, sizeof(SerialRingBuffer*));
			pPacket->SubRef();
		}
	}

	EnterCriticalSection(&CS_SessionIndex);
	SessionIndex.push(Index);
	LeaveCriticalSection(&CS_SessionIndex);

	return SessionID;
}

Session* CWanServerRoom2::SessionFindAndLock(DWORD64 SessionID)
{
	Session* pSession = SessionFind(SessionID);
	int retIOCount = InterlockedIncrement(&pSession->IOCount);
	if (IOCOUNT_FLAG_HIGH(retIOCount) == 1)
	{
		if (0 == InterlockedDecrement(&pSession->IOCount))
			SessionRelease(pSession);
		return nullptr;
	}
	else
	{
		// 검색한 후 SessionID에 해당하는 세션이 Release되어 다른 세션으로 사용된 경우
		if (SessionID != pSession->SessionID)
		{
			if (0 == InterlockedDecrement(&pSession->IOCount))
				SessionRelease(pSession);
			return nullptr;
		}
	}

	return pSession;
}

void CWanServerRoom2::SessionUnLock(Session* pSession)
{
	if (0 == InterlockedDecrement(&pSession->IOCount))
		SessionRelease(pSession);

	return;
}

void CWanServerRoom2::SessionUnLock(DWORD64 SessionID)
{
	Session* pSession = SessionFind(SessionID);
	if (0 == InterlockedDecrement(&pSession->IOCount))
		SessionRelease(pSession);

	return;
}

bool CWanServerRoom2::PqcsProc(PqcsEvent Event, ULONG_PTR p)
{
	switch (Event)
	{
	case PqcsEvent::THREAD_DONE:
		PostQueuedCompletionStatus(hCP, PQCS_TRANSFERRED, 0, reinterpret_cast<LPOVERLAPPED>(PqcsEvent::THREAD_DONE));
		return false;
	case PqcsEvent::SEND_POST:
	{
		Session* pSession = reinterpret_cast<Session*>(p);
		SendPost(pSession);
		if (0 == InterlockedDecrement(&pSession->IOCount))
			SessionRelease(pSession);
	}break;

	case PqcsEvent::SEND_POST_DISCONNECT:
	{
		Session* pSession = reinterpret_cast<Session*>(p);
		SendPost(pSession);
		
		// pq insert합니다.
		ULONGLONG ExecuteTime = GetTickCount64() + pSession->SendDisconnectTime * 1000;
		EnterCriticalSection(&CS_DisconnectUpdatePQueue);
		DisconnectUpdatePQueue.push({ ExecuteTime, pSession->SessionID });
		LeaveCriticalSection(&CS_DisconnectUpdatePQueue);

		if (0 == InterlockedDecrement(&pSession->IOCount))
			SessionRelease(pSession);
	}break;

	case PqcsEvent::ROOM_UPDATE:
	{
		int FrameTime;
		LARGE_INTEGER Begin;
		LARGE_INTEGER End;

		AcquireSRWLockShared(&SRW_RoomMap);
		DWORD RoomNo = static_cast<DWORD>(p);
		auto iter = RoomMap.find(RoomNo);
		if (iter == RoomMap.end())
		{
			ReleaseSRWLockShared(&SRW_RoomMap);
			break;
		}
		CRoom2* pRoom = iter->second;
		EnterCriticalSection(&pRoom->CS_Room);
		ReleaseSRWLockShared(&SRW_RoomMap);

		QueryPerformanceCounter(&Begin);
		pRoom->RoomUpdate();
		FrameTime = pRoom->FrameTime * Frequency.QuadPart / 1000;
		LeaveCriticalSection(&pRoom->CS_Room);

		// RoomUpdatePQueue에 push
		EnterCriticalSection(&CS_RoomUpdatePQueue);
		QueryPerformanceCounter(&End);

		LARGE_INTEGER NextUpdateTime;
		long long DeltaTime = FrameTime - (End.QuadPart - Begin.QuadPart);
		if (DeltaTime >= 0)
			NextUpdateTime.QuadPart = Begin.QuadPart + DeltaTime + FrameTime;
		else
			NextUpdateTime.QuadPart = 0;

		RoomUpdatePQueue.push({ NextUpdateTime, RoomNo });
		LeaveCriticalSection(&CS_RoomUpdatePQueue);
	}break;

	case PqcsEvent::ROOM_DELETE:
		ExecuteDeleteRoom(static_cast<DWORD>(p));
		break;
	case PqcsEvent::SEND_ALLROOM:
	{
		Param param;
		param.value = p;
		ExecuteSendToAllRoom(param);
	}break;

	default:
		// 예상치 못한 PQCS가 왔을 경우
		LOG(L"Network_WanServer", CLog::LEVEL_ERROR,
			L"Unexpected PQCS [PqcsEvent = %lld]",
			static_cast<unsigned long long>(Event));
		Crash();
	}

	return true;
}


void CWanServerRoom2::OutputDebugStringF(LPCTSTR format, ...)
{
	WCHAR buffer[1000] = { 0 };

	va_list arg; // 가변인수 벡터

	va_start(arg, format);
	vswprintf_s(buffer, 1000, format, arg);
	va_end(arg);

	OutputDebugString(buffer);
	return;
}

void CWanServerRoom2::Crash()
{
	// 이 코드는 null 포인터를 역참조하여 크래시를 발생시킵니다.
	int* Nint = NULL;
	*Nint = 0;
}

void CWanServerRoom2::RegisterTimer(function<void(void)> function, DWORD dwMilliseconds)
{
	long idx = InterlockedAdd(&TimerCount, 1);
	if (idx >= MAXIMUM_WAIT_OBJECTS)
	{
		LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"TimerList Lack");
		Crash();
	}

	// 타이머 생성
	hTimers[idx] = CreateWaitableTimer(NULL, FALSE, NULL);
	if (hTimers[idx] == NULL)
	{
		int err = GetLastError();
		LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"Failed CreateWaitableTimer() ErrorCode : %d", err);
		Crash();
	}

	LARGE_INTEGER time;
	time.QuadPart = dwMilliseconds * -10000LL;
	TimerFunctionList[TimerCount] = function;
	if (0 == SetWaitableTimer(hTimers[idx], &time, dwMilliseconds, NULL, NULL, 0))
	{
		int err = GetLastError();
		LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"Failed SetWaitableTimer() ErrorCode : %d", err);
		Crash();
	}

	return;
}

unsigned WINAPI CWanServerRoom2::WorkerThreadWrapping(LPVOID p)
{
	CWanServerRoom2* This = (CWanServerRoom2*)p;
	This->WorkerThread();

	return 0;
}
void CWanServerRoom2::WorkerThread()
{
	bool retval;
	DWORD ThreadID = GetCurrentThreadId();

	Session* pSession;
	DWORD cbTransferred;
	OVERLAPPED* Overlapped;

	while (1)
	{
		cbTransferred = 0;
		pSession = nullptr;

		// 비동기 입출력(I/O) 대기
		retval = GetQueuedCompletionStatus(hCP, &cbTransferred,
			(PULONG_PTR)&pSession, (LPOVERLAPPED*)&Overlapped, INFINITE);

		// GQCS 오류로 인한 실패
		if (Overlapped == nullptr && retval == false)
		{
			int Err = GetLastError();
			LOG(L"Network_WanServer", CLog::LEVEL_SYSTEM, L"Failed GQCS ErrorCode : %d", Err);
			PostQueuedCompletionStatus(hCP, PQCS_TRANSFERRED, 0, (LPOVERLAPPED)PqcsEvent::THREAD_DONE);
			break;
		}

		// PQCS 구분 (cbTransferred는 항상 -1으로 두고 Key(pSession), Ovelapped를 바꾼다 ex) PQCS(hcp, -1,0,0))
		if (retval == true && cbTransferred == -1)
		{
			if (PqcsProc(static_cast<PqcsEvent>(reinterpret_cast<unsigned long long>(Overlapped)), reinterpret_cast<ULONG_PTR>(pSession)))
				continue;
			else
				break;
		}

		// 세션 종료 : 클라에서 RST(실패한 I/O에 대한 완료통지를 처리합니다.) or FIN 보낸 경우 
		if (cbTransferred == 0)
		{
			int Err = GetLastError();
			NetworkError(Err, L"Asynchronous I/O");

			Disconnect(pSession);

			// 실패한 완료통지에 대한 IOCount 감소시킵니다.
			if (0 == InterlockedDecrement(&pSession->IOCount))
				SessionRelease(pSession);
			continue;
		}

		// WSARecv 완료   
		if (Overlapped == &pSession->RecvOverlapped)
		{
			// RecvQ rear 이동
			pSession->RecvQ.MoveRear(cbTransferred);

			// 메시지 처리
			NetworkMessageProc(pSession);

			// Recv 다시 걸어 둡니다.
			RecvPost(pSession);
			continue;
		}

		// WSASend 완료
		if (Overlapped == &pSession->SendOverlapped)
		{
			SerialRingBuffer* Packets[800];
			pSession->SendQ.dequeue(&Packets, pSession->SendBufferCount * sizeof(SerialRingBuffer*));
			for (int iCnt = 0; iCnt < pSession->SendBufferCount; ++iCnt)
			{
				SerialRingBuffer* Packet = Packets[iCnt];
				Packet->SubRef();
			}
			pSession->SendBufferCount = 0;

			// SendFlag 해제
			InterlockedExchange(&pSession->SendFlag, 0);
			if (0 != pSession->SendQ.GetUseSize())
				SendPost(pSession);

			// 성공한 완료통지에 대한 IOCount 감소시킵니다.
			if (0 == InterlockedDecrement(&pSession->IOCount))
				SessionRelease(pSession);
		}
	}

	std::cout << "worker thread 종료" << std::endl;
	return;
}

unsigned WINAPI CWanServerRoom2::TimerThreadWrapping(LPVOID p)
{
	CWanServerRoom2* This = (CWanServerRoom2*)p;
	This->TimerThread();

	return 0;
}
void CWanServerRoom2::TimerThread()
{
	while (TimerEndFlag)
	{
		DWORD Index = WaitForMultipleObjects(TimerCount + 1, hTimers, FALSE, INFINITE);
		if (Index == WAIT_FAILED)
		{
			int Err = GetLastError();
			LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"Fail TimerThread() ErrorCode : %d", Err);
			Crash();
		}
		TimerFunctionList[Index]();
	}
	std::cout << "timer thread 종료" << std::endl;
	return;
}

unsigned WINAPI CWanServerRoom2::AcceptThreadWrapping(LPVOID p)
{
	CWanServerRoom2* This = (CWanServerRoom2*)p;
	This->AcceptThread();

	return 0;
}
void CWanServerRoom2::AcceptThread()
{
	static DWORD SessionID = 1;
	DWORD ThreadID = GetCurrentThreadId();

	SOCKET ClientSocket;
	SOCKADDR_IN ClientAddr;
	int AddrLen = sizeof(ClientAddr);

	while (1)
	{
		ZeroMemory(&ClientSocket, sizeof(ClientSocket));

		// accept()
		ClientSocket = accept(ListenSocket, (SOCKADDR*)&ClientAddr, &AddrLen);
		if (ClientSocket == INVALID_SOCKET)
		{
			int err = WSAGetLastError();
			if (err == WSAEINTR)	// accept 블락 중에 ListenSocket이 CloseHandle 할 때 나오는 에러(AccpetThread 종료할 때 사용)
				break;
			else
			{
				LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"Fail accept() ErrorCode : %d", err);
				continue;
			}
		}
		InterlockedIncrement(&Pnd.AcceptTotalCount);

		// IP와 port 세팅
		WCHAR IP[16] = { 0, };
		WORD Port;
		Port = ntohs(ClientAddr.sin_port);
		InetNtop(AF_INET, &ClientAddr.sin_addr, IP, 16);
		if (OnAcceptRequest(IP, Port) == false)
			continue;


		// RST 설정
		LINGER lingeropt;
		lingeropt.l_linger = 0;
		lingeropt.l_onoff = 1;
		setsockopt(ClientSocket, SOL_SOCKET, SO_LINGER, (char*)&lingeropt, sizeof(lingeropt));

		// 비동기 I/O를 위한 TCP SendBufSize = 0
		if (serverSetting.ZeroCopySend)
		{
			int optval = 0;
			setsockopt(ClientSocket, SOL_SOCKET, SO_SNDBUF, (char*)&optval, sizeof(optval));
		}

		// 세션 구조체 할당 및 초기화
		Session* pSession = SessionAlloc(ClientSocket, SessionID++);
		if (pSession == nullptr)
		{
			closesocket(ClientSocket);
			continue;
		}

		// 기본 룸에 세션 넣기
		InterlockedIncrement(&pSession->IOCount);
		EnterRoom(pSession->SessionID, BasicRoomNo, {});

		// 소켓과 입출력 완료 포트 연결
		CreateIoCompletionPort((HANDLE)ClientSocket, hCP, (ULONG_PTR)pSession, 0);

		// 비동기 입출력 시작
		RecvPost(pSession);

		// Monitor용
		InterlockedIncrement(&Pnd.AcceptTPS);
	}

	// AcceptThread 종료
	LOG(L"Network_WanServer", CLog::LEVEL_SYSTEM, L"AcceptThread Release");
	std::cout << "accept thread 종료" << std::endl;
	return;
}

//---------------------------------------------------------------------------

void CWanServerRoom2::DeleteRoom(DWORD RoomNo)
{
	PostQueuedCompletionStatus(
		hCP,
		PQCS_TRANSFERRED,
		static_cast<ULONG_PTR>(RoomNo),
		reinterpret_cast<LPOVERLAPPED>(PqcsEvent::ROOM_DELETE)
	);
	return;
}
void CWanServerRoom2::ExecuteDeleteRoom(DWORD RoomNo)
{
	CRoom2* pRoom = nullptr;
	while (1)
	{
		AcquireSRWLockExclusive(&SRW_RoomMap);
		{
			auto iter = RoomMap.find(RoomNo);
			if (iter == RoomMap.end())
			{
				ReleaseSRWLockExclusive(&SRW_RoomMap);
				break;
			}

			// 다른 스레드에서 pRoom을 사용하지 않는 것을 보장(RoomUpdate)
			if (!TryEnterCriticalSection(&pRoom->CS_Room))
			{
				ReleaseSRWLockExclusive(&SRW_RoomMap);
				continue;
			}
			pRoom = iter->second;
			RoomMap.erase(RoomNo);
			LeaveCriticalSection(&pRoom->CS_Room);
		}
		ReleaseSRWLockExclusive(&SRW_RoomMap);
	}

	// RoomQueue1 입장 처리
	while (1)
	{
		if (!pRoom->RoomMessageQ.empty())
			break;
		Job_RoomMessageQ Job = pRoom->RoomMessageQ.front();
		pRoom->RoomMessageQ.pop();

		switch (Job.SessionID)
		{
		case INVALID_SESSIONID:	// RoomMsg 요청
		{
			pRoom->OnRecvToRoom(Job.param);
		}break;

		default:				// Enter 요청
		{
			// RoomThread의 MessageQ에 Enter 요청이 오면 IOCount 올리고 입장 처리
			Session* pSession = SessionFindAndLock(Job.SessionID);
			if (pSession == nullptr)
				continue;
			pSession->RoomNo = RoomNo;
			pRoom->SessionMap.insert(std::make_pair(Job.SessionID, pSession));
			pRoom->OnEnterRoom(Job.SessionID, Job.param);
		}break;

		}
	}
	// RoomQueue2 입장 처리
	while (1)
	{
		if (!pRoom->RoomWorkingQ.empty())
			break;
		Job_RoomMessageQ Job = pRoom->RoomWorkingQ.front();
		pRoom->RoomWorkingQ.pop();

		switch (Job.SessionID)
		{
		case INVALID_SESSIONID:	// RoomMsg 요청
		{
			pRoom->OnRecvToRoom(Job.param);
		}break;

		default:				// Enter 요청
		{
			// RoomThread의 MessageQ에 Enter 요청이 오면 IOCount 올리고 입장 처리
			Session* pSession = SessionFindAndLock(Job.SessionID);
			if (pSession == nullptr)
				continue;
			pSession->RoomNo = RoomNo;
			pRoom->SessionMap.insert(std::make_pair(Job.SessionID, pSession));
			pRoom->OnEnterRoom(Job.SessionID, Job.param);
		}break;

		}
	}

	// 기존에 남아 있는 Session들 처리
	for (auto iter = pRoom->SessionMap.begin(); iter != pRoom->SessionMap.end();)
	{
		Session* pSession = iter->second;
		++iter;
		pRoom->CalledChangeRoom = false;

		// 남아 있는 Session 처리 (OnSessionKickOut() 내부에서 따로 ChangeRoom을 안 했을 경우 해당 Session을 삭제 시킨다.
		pRoom->OnSessionKickOut(pSession->SessionID);

		// OnSessionKickOut() 안에서 ChagneRoom을 한 경우
		if (pRoom->CalledChangeRoom)
			continue;

		// 이미 끊어진 Session or OnSessionKickOut() 안에서 Disconnect을 한 경우
		if (pSession->DisconnectFlag != 1)
			Disconnect(pSession);
		pRoom->SessionMap.erase(pSession->SessionID);
		pRoom->OnSessionRelease(pSession->SessionID);
		SessionUnLock(pSession);
	}

	delete pRoom;

	return;
}

void CWanServerRoom2::EnterRoom(DWORD64 SessionID, DWORD RoomNo, Param param)
{
	AcquireSRWLockShared(&SRW_RoomMap);
	{
		auto iter = RoomMap.find(RoomNo);
		if (iter != RoomMap.end())
		{
			CRoom2* pRoom = iter->second;
			EnterCriticalSection(&pRoom->CS_RoomMessageQ);
			pRoom->MessageQ->push({ en_JOB_ROOM::ENTER, SessionID, param });
			LeaveCriticalSection(&pRoom->CS_RoomMessageQ);
		}
	}
	ReleaseSRWLockShared(&SRW_RoomMap);

	return;
}

void CWanServerRoom2::SendToAllRoom(Param param)
{
	ULONG_PTR dwCompletionKey = param.value;
	PostQueuedCompletionStatus(
		hCP,
		PQCS_TRANSFERRED, 
		dwCompletionKey,
		reinterpret_cast<LPOVERLAPPED>(PqcsEvent::SEND_ALLROOM)
	);

	return;
}
void CWanServerRoom2::ExecuteSendToAllRoom(Param param)
{
	AcquireSRWLockShared(&SRW_RoomMap);
	{
		for (auto& iter : RoomMap)
		{
			CRoom2* pRoom = iter.second;
			EnterCriticalSection(&pRoom->CS_RoomMessageQ);
			pRoom->MessageQ->push({ en_JOB_ROOM::ROOM_MESSAGE, INVALID_SESSIONID, param });
			LeaveCriticalSection(&pRoom->CS_RoomMessageQ);
		}
	}
	ReleaseSRWLockShared(&SRW_RoomMap);
	return;
}

bool CWanServerRoom2::SendToRoom(DWORD RoomNo, Param param)
{
	AcquireSRWLockShared(&SRW_RoomMap);
	{
		auto iter = RoomMap.find(RoomNo);
		if (iter == RoomMap.end())
		{
			ReleaseSRWLockShared(&SRW_RoomMap);
			return false;
		}
		CRoom2* pRoom = iter->second;
		EnterCriticalSection(&pRoom->CS_RoomMessageQ);
		pRoom->MessageQ->push({ en_JOB_ROOM::ROOM_MESSAGE, INVALID_SESSIONID, param });
		LeaveCriticalSection(&pRoom->CS_RoomMessageQ);
	}
	ReleaseSRWLockShared(&SRW_RoomMap);
	return true;
}

bool CWanServerRoom2::SendToRoomSession(DWORD64 SessionID, Param param)
{
	DWORD roomNo = GetRoomNo(SessionID);
	if (roomNo == INVALID_ROOMNO)
		return false;

	AcquireSRWLockShared(&SRW_RoomMap);
	{
		auto iter = RoomMap.find(roomNo);
		if (iter == RoomMap.end())
		{
			ReleaseSRWLockShared(&SRW_RoomMap);
			return false;
		}

		CRoom2* pRoom = iter->second;
		EnterCriticalSection(&pRoom->CS_RoomMessageQ);
		pRoom->MessageQ->push({ en_JOB_ROOM::ROOM_MESSAGE_SESSION, SessionID, param });
		LeaveCriticalSection(&pRoom->CS_RoomMessageQ);
	}
	ReleaseSRWLockShared(&SRW_RoomMap);
	return true;
}

void CWanServerRoom2::FlushRoom()
{
	while (1)
	{
		LARGE_INTEGER CurrTime;
		QueryPerformanceCounter(&CurrTime);
		EnterCriticalSection(&CS_RoomUpdatePQueue);
		if (RoomUpdatePQueue.empty())
		{
			LeaveCriticalSection(&CS_RoomUpdatePQueue);
			break;
		}
		auto job = RoomUpdatePQueue.top();
		if (job.ExecuteTime.QuadPart >= CurrTime.QuadPart)
		{
			LeaveCriticalSection(&CS_RoomUpdatePQueue);
			break;
		}
		RoomUpdatePQueue.pop();
		LeaveCriticalSection(&CS_RoomUpdatePQueue);

		PostQueuedCompletionStatus(
			hCP,
			PQCS_TRANSFERRED,
			static_cast<ULONG_PTR>(job.RoomNo),
			reinterpret_cast<LPOVERLAPPED>(PqcsEvent::ROOM_UPDATE));
	}
}

void CWanServerRoom2::SetBasicRoom(DWORD RoomNo)
{
	BasicRoomNo = RoomNo;
	return;
}

DWORD CWanServerRoom2::GetRoomNo(DWORD64 SessionID)
{
	DWORD RoomNo = INVALID_ROOMNO;

	Session* pSession = SessionFindAndLock(SessionID);
	if (pSession != nullptr)
		RoomNo = pSession->RoomNo;
	SessionUnLock(SessionID);

	return RoomNo;
}