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

	// CS ����ü �ʱ�ȭ
	InitializeCriticalSection(&CS_SessionIndex);
	InitializeCriticalSection(&CS_RoomUpdatePQueue);
	InitializeCriticalSection(&CS_DisconnectUpdatePQueue);
	InitializeSRWLock(&SRW_RoomMap);

	// RoomTimer ���
	auto bindfunction = std::bind(&CWanServerRoom2::FlushRoom, this);
	RegisterTimer(bindfunction, 20);

	auto bindfunction2 = std::bind(&CWanServerRoom2::FlushDisconnectSession, this);
	RegisterTimer(bindfunction2, 1000);
}

bool CWanServerRoom2::Start(const WCHAR* IP, WORD Port, WORD NumberOfCreateThreads, WORD NumberOfConcurrentThreads, bool Nagle, bool ZeroCopySend, BYTE PacketCode, BYTE PacketKey, WORD MaxSessionCount)
{
	// wprintf�� �ѱ� ���
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

	// ��Ʈ��ũ ��� PacketKey Set
	SerialRingBuffer::SetPacketKey(PacketKey);

	//Session ����
	Sessions = new(std::nothrow) Session[serverSetting.SessionMax];
	if (Sessions == NULL)
	{
		LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"Fail Session Alloc");
		return false;
	}

	// SessionArr�� ����ִ� �ε��� �ֱ�
	for (int iCnt = serverSetting.SessionMax - 1; iCnt >= 0; --iCnt)
	{
		SessionIndex.push(iCnt);
	}

	if (BasicRoomNo == INVALID_ROOMNO)
	{
		LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"Not Setting BasicRoomNo");
		return false;
	}

	// ����� �Ϸ� ��Ʈ ����
	hCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, serverSetting.ActiveThread);
	if (hCP == NULL)
	{
		int Err = GetLastError();
		LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"Fail CreateIoCompletionPort() ErrorCode : %d", Err);
		return false;
	}

	// IOCP ��Ŀ������ ����
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

	// listen socket �ʱ�ȭ
	ListenSocketInit(IP, Port, Nagle);

	// Accept ������ ����
	hAccept_Threads = (HANDLE)_beginthreadex(nullptr, 0, AcceptThreadWrapping, this, 0, nullptr);
	if (hAccept_Threads == 0)
	{
		int Err = errno;
		LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"Fail _beginthreadex() ErrorCode : %d", Err);
		return false;
	}

	// Ÿ�̸� ������ ����
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
	// wprintf�� �ѱ� ���
	_wsetlocale(LC_ALL, L"korean");
	timeBeginPeriod(1);

	serverSetting = _ServerSetting;

	// ��Ʈ��ũ ��� PacketKey Set
	SerialRingBuffer::SetPacketKey(serverSetting.PacketKey);

	//Session ����
	Sessions = new(std::nothrow) Session[serverSetting.SessionMax];
	if (Sessions == NULL)
	{
		LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"Fail Session Alloc");
		return false;
	}

	// SessionArr�� ����ִ� �ε��� �ֱ�
	for (int iCnt = serverSetting.SessionMax - 1; iCnt >= 0; --iCnt)
	{
		SessionIndex.push(iCnt);
	}

	if (BasicRoomNo == INVALID_ROOMNO)
	{
		LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"Not Setting BasicRoomNo");
		return false;
	}

	// ����� �Ϸ� ��Ʈ ����
	hCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, serverSetting.ActiveThread);
	if (hCP == NULL)
	{
		int Err = GetLastError();
		LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"Fail CreateIoCompletionPort() ErrorCode : %d", Err);
		return false;
	}
	
	// IOCP ��Ŀ������ ����
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

	// listen socket �ʱ�ȭ
	ListenSocketInit(serverSetting.IP_wchar, serverSetting.Port, serverSetting.Nagle);
	// Accept ������ ����
	hAccept_Threads = (HANDLE)_beginthreadex(nullptr, 0, AcceptThreadWrapping, this, 0, nullptr);
	if (hAccept_Threads == 0)
	{
		int Err = errno;
		LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"Fail _beginthreadex() ErrorCode : %d", Err);
		return false;
	}

	// Ÿ�̸� ������ ����
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
	// accpet ������ ����
	closesocket(ListenSocket);
	if (WAIT_FAILED == WaitForSingleObject(hAccept_Threads, INFINITE))
	{
		int err = GetLastError();
		LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"Failed to close accept thread : ErrorCode : %d", err);
		Crash();
	}
	CloseHandle(hAccept_Threads);

	// session ���� ó��
	for (int idx = 0; idx < serverSetting.SessionMax; ++idx)
	{
		if(Sessions[idx].SessionID != INVALID_SESSIONID)
			Disconnect(Sessions[idx].SessionID);
	}
	while (SessionIndex.size() != serverSetting.SessionMax)
	{
		Sleep(1000);
	}

	// timer thread ����
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
	

	// worker ������ ����
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

	// Room ����
	for (auto& room : RoomMap)
	{
		CRoom2* pRoom = room.second;
		delete pRoom;
	}

	// session ����
	delete[] Sessions;

	// lock ��ü ����
	DeleteCriticalSection(&CS_SessionIndex);
	DeleteCriticalSection(&CS_RoomUpdatePQueue);
	DeleteCriticalSection(&CS_DisconnectUpdatePQueue);

	// Ÿ�̸� ���� �ʱ�ȭ
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
		if (SessionID != pSession->SessionID)	// �˻��� �� SessionID�� �ش��ϴ� ������ Release�Ǿ� �ٸ� �������� ���� ���
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
		if (SessionID != pSession->SessionID)	// �˻��� �� SessionID�� �ش��ϴ� ������ Release�Ǿ� �ٸ� �������� ���� ���
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

	// SendQ�� ���� ũ�Ⱑ ������ ���
	// Ŭ�� �� TCP���� ���۰� �� ���� ���� �� TCP�۽� ���۰� ���� �� ��Ȳ -> Ŭ�� ������ recv�� ���ϰ� �ִ� ���
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

	// Monitor��
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
		if (SessionID != pSession->SessionID)	// �˻��� �� SessionID�� �ش��ϴ� ������ Release�Ǿ� �ٸ� �������� ���� ���
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

	// SendQ�� ���� ũ�Ⱑ ������ ���
	// Ŭ�� �� TCP���� ���۰� �� ���� ���� �� TCP�۽� ���۰� ���� �� ��Ȳ -> Ŭ�� ������ recv�� ���ϰ� �ִ� ���
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

	// Monitor��
	InterlockedIncrement(&Pnd.SendTPS);
	return true;
}
// PQCS ����
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
		if (SessionID != pSession->SessionID)	// �˻��� �� SessionID�� �ش��ϴ� ������ Release�Ǿ� �ٸ� �������� ���� ���
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

	// SendQ�� ���� ũ�Ⱑ ������ ���
	// Ŭ�� �� TCP���� ���۰� �� ���� ���� �� TCP�۽� ���۰� ���� �� ��Ȳ -> Ŭ�� ������ recv�� ���ϰ� �ִ� ���
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

		// ���� SendFlag�� �����Դ� (PQCS�� �ؾ��Ѵ�) 
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


	// Monitor��
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
		if (SessionID != pSession->SessionID)	// �˻��� �� SessionID�� �ش��ϴ� ������ Release�Ǿ� �ٸ� �������� ���� ���
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

	// SendQ�� ���� ũ�Ⱑ ������ ���
	// Ŭ�� �� TCP���� ���۰� �� ���� ���� �� TCP�۽� ���۰� ���� �� ��Ȳ -> Ŭ�� ������ recv�� ���ϰ� �ִ� ���
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
	pSession->SendDisconnectTime = dwSeconds;	// Diconnect Time ����

	PostQueuedCompletionStatus(hCP, PQCS_TRANSFERRED, reinterpret_cast<ULONG_PTR>(pSession), reinterpret_cast<LPOVERLAPPED>(PqcsEvent::SEND_POST_DISCONNECT));

	// Monitor��
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
	// ���� �ʱ�ȭ
	WSADATA WSA;
	if (WSAStartup(MAKEWORD(2, 2), &WSA) != 0)
	{
		int Err = WSAGetLastError();
		LOG(L"Network_WanServer", CLog::LEVEL_ERROR,
			L"Fail WSAStartup() ErrorCode : %d", Err);
		return false;
	}
	LOG(L"Network_WanServer", CLog::LEVEL_SYSTEM, L"WSAStartup()");


	// Listen ���� ����
	ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ListenSocket == INVALID_SOCKET)
	{
		int Err = WSAGetLastError();
		LOG(L"Network_WanServer", CLog::LEVEL_ERROR,
			L"Fail ListenSocket() ErrorCode : %d", Err);
		return false;
	}
	LOG(L"Network_WanServer", CLog::LEVEL_SYSTEM, L"ListenSocket()");

	//���̱� �˰��� on/off
	int option = Nagle;
	setsockopt(ListenSocket,  // �ش� ����
		IPPROTO_TCP,          // ������ ����
		TCP_NODELAY,          // ���� �ɼ�
		(const char*)&option, // �ɼ� ������
		sizeof(option));      // �ɼ� ũ��

	// Listen ���� �ּ� �ʱ�ȭ
	SOCKADDR_IN ListenSockAddr;
	ZeroMemory(&ListenSockAddr, sizeof(ListenSockAddr));
	ListenSockAddr.sin_family = AF_INET;
	InetPton(AF_INET, IP, &ListenSockAddr.sin_addr);
	ListenSockAddr.sin_port = htons(Port);

	// ���ε� : std::bind�� ��ġ�� ������ window�� bind�� �����̹Ƿ� �̸����� ���ӽ����̽��� std::bind�� �����Ѵ�.
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
	// Ŭ���̾�Ʈ ���� ���
	SOCKADDR_IN ClientAddr;
	int AddrLen = sizeof(ClientAddr);
	getpeername(pSession->socket, (SOCKADDR*)&ClientAddr, &AddrLen);

	// IP�� port ����
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
		if (SessionID != pSession->SessionID)	// �˻��� �� SessionID�� �ش��ϴ� ������ Release�Ǿ� �ٸ� �������� ���� ���
		{
			if (0 == InterlockedDecrement(&pSession->IOCount))
				SessionRelease(pSession);
			return;
		}
	}

	// Ŭ���̾�Ʈ ���� ���
	SOCKADDR_IN ClientAddr;
	int AddrLen = sizeof(ClientAddr);
	getpeername(pSession->socket, (SOCKADDR*)&ClientAddr, &AddrLen);

	// IP�� port ����
	*Port = ntohs(ClientAddr.sin_port);
	InetNtop(AF_INET, &ClientAddr.sin_addr, IP, 16);

	if (0 == InterlockedDecrement(&pSession->IOCount))
		SessionRelease(pSession);
	return;
}

void CWanServerRoom2::NetworkError(int Err, const WCHAR* FunctionName)
{
	//Err == WSAEINVAL						// 10022 : WSASend���� dwBufferCount �μ��� '0'�� ���� �� ��Ÿ���� ���� Ȯ��

	if (Err != WSAECONNRESET &&				// 10054 : Ŭ�� ������ ������ ���
		Err != WSAECONNABORTED &&			// 10053 : ����Ʈ����(TCP)�� ������ �ߴܵ� ���
		Err != ERROR_NETNAME_DELETED &&		// 64	 : RST, Ŭ�󿡼� ���� ���� ���
		Err != WSAENOTSOCK &&				// 10038 : ������ �ƴ� �׸񿡼� �۾��� �õ��߽��ϴ�.
		Err != ERROR_OPERATION_ABORTED &&	// 995	 : ������ ���� �Ǵ� ���ø����̼� ��û���� ���� I/O �۾��� �ߴܵǾ����ϴ�. (CancelIoEx)
		Err != ERROR_SUCCESS)				// 0	 : �۾��� ���������� �Ϸ�Ǿ����ϴ�. (FIN�� �޾��� ���)
	{
		LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"Fail %s ErrorCode : %d", FunctionName, Err);
	}
	return;
}

void CWanServerRoom2::NetworkMessageProc(Session* pSession)
{
	while (1)
	{
		// NetworkHeader ���� ũ�� �ִ��� Ȯ��
		if (sizeof(NetworkHeader) >= pSession->RecvQ.GetUseSize())
			break;

		// Network Header peek
		NetworkHeader NetworkHeader;
		int retpeek = pSession->RecvQ.peek(&NetworkHeader, sizeof(NetworkHeader));

		// code Ȯ��
		if (NetworkHeader.Code != serverSetting.PacketCode)
		{
			LOG(L"Network_WanServer", CLog::LEVEL_DEBUG, L"Packet Error : Code Mismatch");
			Disconnect(pSession->SessionID);
			break;
		}

		// PayLoad�� ���� ũ�⺸�� ū�� Ȯ��
		if (NetworkHeader.Len > SerialRingBuffer::en_BUFFER_DEFAULT_SIZE - sizeof(NetworkHeader) - 1)
		{
			LOG(L"Network_WanServer", CLog::LEVEL_DEBUG, L"Packet Error : Buffer Size Overflow");
			Disconnect(pSession->SessionID);
			break;
		}

		// PayLoad�� ũ�Ⱑ 0 �� ���
		if (NetworkHeader.Len == 0)
		{
			LOG(L"Network_WanServer", CLog::LEVEL_DEBUG, L"Packet Error : No PayLoad");
			Disconnect(pSession->SessionID);
			break;
		}

		// Session�� MessageQ�� ���� ũ�Ⱑ ���� �� ���
		if (sizeof(SerialRingBuffer*) > pSession->MessageQ.GetFreeSize())
		{
			LOG(L"Network_WanServer", CLog::LEVEL_DEBUG, L"Lack Session MessageQ buffer");
			Disconnect(pSession->SessionID);
			break;
		}


		// PayLoad�� ���Դ��� Ȯ��
		if (pSession->RecvQ.GetUseSize() - sizeof(NetworkHeader) >= NetworkHeader.Len)
		{
			// Network ��� ũ�⸸ŭ �̵�
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

			// decode ����
			if (!Packet->decode())
			{
				LOG(L"Network_WanServer", CLog::LEVEL_DEBUG, L"Failed decode()");
				Packet->Free();
				Disconnect(pSession->SessionID);
				break;
			}

			// SerialRingBuffer ���
			Packet->AddRef();
			pSession->MessageQ.enqueue(&Packet, sizeof(SerialRingBuffer*));

			// Monitor ��
			InterlockedIncrement(&Pnd.RecvTPS);
		}
		else break;
	}

	return;
}

void CWanServerRoom2::RecvPost(Session* pSession)
{
	// �񵿱� ����� ����
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
			// WSARecv�� ���� ���� ��� IOCount ����
			if (0 == InterlockedDecrement(&pSession->IOCount))
				SessionRelease(pSession);

			// Socket�� ������ �������� �̰����� ������ �Ǹ� Recv���� �Ϸ� ������ �� �޴´� send�ʿ��� ���� �Ϸ� ������ �������� Disconnect�� �Ǿ� DisconnectFlag�� �ö󰡸� ������µ� �� �� �ƴ� ���
			// Room�� �ִ� Session�� DisconnectFlag�� ���� ���ؼ� ���� ��������ʴ´�. ���� �� ������ DisconnectFlag�� �÷��ش�.
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
	Send�� 1ȸ ����
	1ȸ�� ���� ���� ������ WSASend �Ϸ������� �ͼ� �������� front�� ó�� �Ǳ� ���� WSARecv �Ϸ�������
	���� �ͼ� �޽��� ó�� �� WSARecv�Ϸ����� �ʿ��� �ٽ� �ѹ� WSASend�� �ϰ� �Ǹ� ���� WSASend�� front��
	ó�� �Ǳ� ���� Send�� �ϰ� �Ǿ� �޽����� ��ø�Ȱ� ������ �� �ִ�.
	*/
	SerialRingBuffer* SendQ = &pSession->SendQ;

	/*
		���� ���������� UseSize�� ������� ������, SendMSG�� ���� SendPost�� ȣ�� �Ǹ� Session�� lock�� �ɷ��־ ��� ������
		WorkerThread���� Send�Ϸ��������� SendPost�� ȣ�� �ϴ� ��� Session�� lock�� �ɷ����� �ʱ� ������ ��� ���ο��� GetUs
		esize()�� ũ�Ⱑ �޶��� �� �ִ�.
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

	// SerialRingBuffer�� point�� SendQ�� �ֽ��ϴ�.
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

	// IOCount�� ����
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


	// SendQ�� SerialRingBuffer RefCount Sub �� SendQ���� �����
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

	// SerialRingBuffer ���
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
		// �˻��� �� SessionID�� �ش��ϴ� ������ Release�Ǿ� �ٸ� �������� ���� ���
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
		
		// pq insert�մϴ�.
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

		// RoomUpdatePQueue�� push
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
		// ����ġ ���� PQCS�� ���� ���
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

	va_list arg; // �����μ� ����

	va_start(arg, format);
	vswprintf_s(buffer, 1000, format, arg);
	va_end(arg);

	OutputDebugString(buffer);
	return;
}

void CWanServerRoom2::Crash()
{
	// �� �ڵ�� null �����͸� �������Ͽ� ũ���ø� �߻���ŵ�ϴ�.
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

	// Ÿ�̸� ����
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

		// �񵿱� �����(I/O) ���
		retval = GetQueuedCompletionStatus(hCP, &cbTransferred,
			(PULONG_PTR)&pSession, (LPOVERLAPPED*)&Overlapped, INFINITE);

		// GQCS ������ ���� ����
		if (Overlapped == nullptr && retval == false)
		{
			int Err = GetLastError();
			LOG(L"Network_WanServer", CLog::LEVEL_SYSTEM, L"Failed GQCS ErrorCode : %d", Err);
			PostQueuedCompletionStatus(hCP, PQCS_TRANSFERRED, 0, (LPOVERLAPPED)PqcsEvent::THREAD_DONE);
			break;
		}

		// PQCS ���� (cbTransferred�� �׻� -1���� �ΰ� Key(pSession), Ovelapped�� �ٲ۴� ex) PQCS(hcp, -1,0,0))
		if (retval == true && cbTransferred == -1)
		{
			if (PqcsProc(static_cast<PqcsEvent>(reinterpret_cast<unsigned long long>(Overlapped)), reinterpret_cast<ULONG_PTR>(pSession)))
				continue;
			else
				break;
		}

		// ���� ���� : Ŭ�󿡼� RST(������ I/O�� ���� �Ϸ������� ó���մϴ�.) or FIN ���� ��� 
		if (cbTransferred == 0)
		{
			int Err = GetLastError();
			NetworkError(Err, L"Asynchronous I/O");

			Disconnect(pSession);

			// ������ �Ϸ������� ���� IOCount ���ҽ�ŵ�ϴ�.
			if (0 == InterlockedDecrement(&pSession->IOCount))
				SessionRelease(pSession);
			continue;
		}

		// WSARecv �Ϸ�   
		if (Overlapped == &pSession->RecvOverlapped)
		{
			// RecvQ rear �̵�
			pSession->RecvQ.MoveRear(cbTransferred);

			// �޽��� ó��
			NetworkMessageProc(pSession);

			// Recv �ٽ� �ɾ� �Ӵϴ�.
			RecvPost(pSession);
			continue;
		}

		// WSASend �Ϸ�
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

			// SendFlag ����
			InterlockedExchange(&pSession->SendFlag, 0);
			if (0 != pSession->SendQ.GetUseSize())
				SendPost(pSession);

			// ������ �Ϸ������� ���� IOCount ���ҽ�ŵ�ϴ�.
			if (0 == InterlockedDecrement(&pSession->IOCount))
				SessionRelease(pSession);
		}
	}

	std::cout << "worker thread ����" << std::endl;
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
	std::cout << "timer thread ����" << std::endl;
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
			if (err == WSAEINTR)	// accept ��� �߿� ListenSocket�� CloseHandle �� �� ������ ����(AccpetThread ������ �� ���)
				break;
			else
			{
				LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"Fail accept() ErrorCode : %d", err);
				continue;
			}
		}
		InterlockedIncrement(&Pnd.AcceptTotalCount);

		// IP�� port ����
		WCHAR IP[16] = { 0, };
		WORD Port;
		Port = ntohs(ClientAddr.sin_port);
		InetNtop(AF_INET, &ClientAddr.sin_addr, IP, 16);
		if (OnAcceptRequest(IP, Port) == false)
			continue;


		// RST ����
		LINGER lingeropt;
		lingeropt.l_linger = 0;
		lingeropt.l_onoff = 1;
		setsockopt(ClientSocket, SOL_SOCKET, SO_LINGER, (char*)&lingeropt, sizeof(lingeropt));

		// �񵿱� I/O�� ���� TCP SendBufSize = 0
		if (serverSetting.ZeroCopySend)
		{
			int optval = 0;
			setsockopt(ClientSocket, SOL_SOCKET, SO_SNDBUF, (char*)&optval, sizeof(optval));
		}

		// ���� ����ü �Ҵ� �� �ʱ�ȭ
		Session* pSession = SessionAlloc(ClientSocket, SessionID++);
		if (pSession == nullptr)
		{
			closesocket(ClientSocket);
			continue;
		}

		// �⺻ �뿡 ���� �ֱ�
		InterlockedIncrement(&pSession->IOCount);
		EnterRoom(pSession->SessionID, BasicRoomNo, {});

		// ���ϰ� ����� �Ϸ� ��Ʈ ����
		CreateIoCompletionPort((HANDLE)ClientSocket, hCP, (ULONG_PTR)pSession, 0);

		// �񵿱� ����� ����
		RecvPost(pSession);

		// Monitor��
		InterlockedIncrement(&Pnd.AcceptTPS);
	}

	// AcceptThread ����
	LOG(L"Network_WanServer", CLog::LEVEL_SYSTEM, L"AcceptThread Release");
	std::cout << "accept thread ����" << std::endl;
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

			// �ٸ� �����忡�� pRoom�� ������� �ʴ� ���� ����(RoomUpdate)
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

	// RoomQueue1 ���� ó��
	while (1)
	{
		if (!pRoom->RoomMessageQ.empty())
			break;
		Job_RoomMessageQ Job = pRoom->RoomMessageQ.front();
		pRoom->RoomMessageQ.pop();

		switch (Job.SessionID)
		{
		case INVALID_SESSIONID:	// RoomMsg ��û
		{
			pRoom->OnRecvToRoom(Job.param);
		}break;

		default:				// Enter ��û
		{
			// RoomThread�� MessageQ�� Enter ��û�� ���� IOCount �ø��� ���� ó��
			Session* pSession = SessionFindAndLock(Job.SessionID);
			if (pSession == nullptr)
				continue;
			pSession->RoomNo = RoomNo;
			pRoom->SessionMap.insert(std::make_pair(Job.SessionID, pSession));
			pRoom->OnEnterRoom(Job.SessionID, Job.param);
		}break;

		}
	}
	// RoomQueue2 ���� ó��
	while (1)
	{
		if (!pRoom->RoomWorkingQ.empty())
			break;
		Job_RoomMessageQ Job = pRoom->RoomWorkingQ.front();
		pRoom->RoomWorkingQ.pop();

		switch (Job.SessionID)
		{
		case INVALID_SESSIONID:	// RoomMsg ��û
		{
			pRoom->OnRecvToRoom(Job.param);
		}break;

		default:				// Enter ��û
		{
			// RoomThread�� MessageQ�� Enter ��û�� ���� IOCount �ø��� ���� ó��
			Session* pSession = SessionFindAndLock(Job.SessionID);
			if (pSession == nullptr)
				continue;
			pSession->RoomNo = RoomNo;
			pRoom->SessionMap.insert(std::make_pair(Job.SessionID, pSession));
			pRoom->OnEnterRoom(Job.SessionID, Job.param);
		}break;

		}
	}

	// ������ ���� �ִ� Session�� ó��
	for (auto iter = pRoom->SessionMap.begin(); iter != pRoom->SessionMap.end();)
	{
		Session* pSession = iter->second;
		++iter;
		pRoom->CalledChangeRoom = false;

		// ���� �ִ� Session ó�� (OnSessionKickOut() ���ο��� ���� ChangeRoom�� �� ���� ��� �ش� Session�� ���� ��Ų��.
		pRoom->OnSessionKickOut(pSession->SessionID);

		// OnSessionKickOut() �ȿ��� ChagneRoom�� �� ���
		if (pRoom->CalledChangeRoom)
			continue;

		// �̹� ������ Session or OnSessionKickOut() �ȿ��� Disconnect�� �� ���
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