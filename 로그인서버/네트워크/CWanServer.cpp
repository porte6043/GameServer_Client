#include "CWanServer.h"

CWanServer::CWanServer() : SessionIndex(), CS_SessionIndex(), hTimers{}, TimerList{}
{
	Sessions = nullptr;
	DisconnectedSessions = nullptr;
	TimerCount = 0;
	ListenSocket = INVALID_SOCKET;
	Port = 0;
	MaxSessionCount = 0;
	_Nagle = false;
	_ZeroCopySend = false;
	_PacketCode = 0;
	hCP = nullptr;
	hAccept_Threads = nullptr;
	hTimer_Threads = nullptr;
	hWorker_Threads = nullptr;
}
CWanServer::~CWanServer()
{

}

bool CWanServer::Start(const WCHAR* IP, WORD Port, WORD NumberOfCreateThreads, WORD NumberOfConcurrentThreads, bool Nagle, bool ZeroCopySend, BYTE PacketCode, BYTE PacketKey, WORD MaxSessionCount)
{
	// wprintf�� �ѱ� ���
	_wsetlocale(LC_ALL, L"korean");

	// CS ����ü �ʱ�ȭ
	InitializeCriticalSection(&CS_SessionIndex);

	// ��Ʈ��ũ ��� PacketKey Set
	SerialRingBuffer::SetPacketKey(PacketKey);

	this->Port = Port;
	_Nagle = Nagle;
	_ZeroCopySend = ZeroCopySend;
	_PacketCode = PacketCode;
	this->MaxSessionCount = MaxSessionCount;

	// DisconnectedSession ����
	DisconnectedSessions = new DisconnectedSession[MaxSessionCount];

	// Session ����
	Sessions = new Session[MaxSessionCount];

	// SessionArr�� ����ִ� �ε��� �ֱ�
	for (int iCnt = MaxSessionCount - 1; iCnt >= 0; --iCnt)
	{
		SessionIndex.push(iCnt);
	}

	// ����� �Ϸ� ��Ʈ ����
	hCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, NumberOfConcurrentThreads);
	if (hCP == NULL)
	{
		int Err = GetLastError();
		LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"Fail CreateIoCompletionPort() ErrorCode : %d", Err);
		return 1;
	}

	// IOCP ��Ŀ������ ����
	hWorker_Threads = new HANDLE[NumberOfCreateThreads];
	for (unsigned int iCnt = 0; iCnt < NumberOfCreateThreads; ++iCnt)
	{
		hWorker_Threads[iCnt] = (HANDLE)_beginthreadex(nullptr, 0, WorkerThreadWrapping, this, 0, nullptr);
		if (hWorker_Threads[iCnt] == 0)
		{
			int Err = errno;
			LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"Fail _beginthreadex() ErrorCode : %d", Err);
			return false;
		}
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

	// ������ ���� Ÿ�̸� ����
	RegisterTimer(DisconnectSession, this, 1000);

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
bool CWanServer::Start(ServerSetting serverSetting)
{
	// wprintf�� �ѱ� ���
	_wsetlocale(LC_ALL, L"korean");

	// CS ����ü �ʱ�ȭ
	InitializeCriticalSection(&CS_SessionIndex);

	// ��Ʈ��ũ ��� PacketKey Set
	SerialRingBuffer::SetPacketKey(serverSetting.PacketKey);

	this->Port = Port;
	_Nagle = serverSetting.Nagle;
	_ZeroCopySend = serverSetting.ZeroCopySend;
	_PacketCode = serverSetting.PacketCode;
	this->MaxSessionCount = serverSetting.SessionMax;

	// DisconnectedSession ����
	DisconnectedSessions = new DisconnectedSession[serverSetting.SessionMax];

	// Session ����
	Sessions = new Session[serverSetting.SessionMax];

	// SessionArr�� ����ִ� �ε��� �ֱ�
	for (int iCnt = serverSetting.SessionMax - 1; iCnt >= 0; --iCnt)
	{
		SessionIndex.push(iCnt);
	}

	// ����� �Ϸ� ��Ʈ ����
	hCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, serverSetting.ActiveThread);
	if (hCP == NULL)
	{
		int Err = GetLastError();
		LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"Fail CreateIoCompletionPort() ErrorCode : %d", Err);
		return 1;
	}

	// IOCP ��Ŀ������ ����
	hWorker_Threads = new HANDLE[serverSetting.WorkerThread];
	for (unsigned int iCnt = 0; iCnt < serverSetting.WorkerThread; ++iCnt)
	{
		hWorker_Threads[iCnt] = (HANDLE)_beginthreadex(nullptr, 0, WorkerThreadWrapping, this, 0, nullptr);
		if (hWorker_Threads[iCnt] == 0)
		{
			int Err = errno;
			LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"Fail _beginthreadex() ErrorCode : %d", Err);
			return false;
		}
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

	// ������ ���� Ÿ�̸� ����
	RegisterTimer(DisconnectSession, this, 1000);

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
bool CWanServer::Disconnect(DWORD64 SessionID)
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
void CWanServer::Disconnect(Session* pSession)
{
	InterlockedExchange(&pSession->DisconnectFlag, 1);
	CancelIoEx(reinterpret_cast<HANDLE>(pSession->socket), NULL);

	return;
}

bool CWanServer::SendMSG(DWORD64 SessionID, CPacket& packet)
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

	SerialRingBuffer* Packet = packet.Packet;
	NetworkHeader NetworkHeader(_PacketCode ,Packet);
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
bool CWanServer::SendMSG_PQCS(DWORD64 SessionID, CPacket& packet)
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

	SerialRingBuffer* Packet = packet.Packet;
	NetworkHeader NetworkHeader(_PacketCode, Packet);
	Packet->SetWanHeader(&NetworkHeader, sizeof(NetworkHeader));
	Packet->encode();

	Packet->AddRef();
	EnterCriticalSection(&pSession->CS_SendQ);
	pSession->SendQ.enqueue(&Packet, sizeof(SerialRingBuffer*));
	LeaveCriticalSection(&pSession->CS_SendQ);

	PostQueuedCompletionStatus(hCP, -1, (ULONG_PTR)pSession, (LPOVERLAPPED)1);

	// Monitor��
	InterlockedIncrement(&Pnd.SendTPS);
	return true;
}

bool CWanServer::SendMSG_Disconnect(DWORD64 SessionID, CPacket& packet, DWORD dwSeconds)
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

	SerialRingBuffer* Packet = packet.Packet;
	NetworkHeader NetworkHeader(_PacketCode, Packet);
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

	// Disconnect ����
	int SessionIndex = SESSIONID_INDEX_LOW(SessionID);
	InterlockedExchange(&DisconnectedSessions[SessionIndex].dwSeconds, dwSeconds);
	InterlockedExchange64((LONG64*)&DisconnectedSessions[SessionIndex].Tick, GetTickCount64());
	InterlockedExchange64((LONG64*)&DisconnectedSessions[SessionIndex].SessionID, SessionID);

	// Monitor��
	InterlockedIncrement(&Pnd.SendTPS);
	return true;
}

void CWanServer::GetPND(PerformanceNetworkData& pnd)
{
	pnd.SessionCount = MaxSessionCount - SessionIndex.size();
	pnd.AcceptTotalCount = Pnd.AcceptTotalCount;
	pnd.PacketTotalCount = SerialRingBuffer::GetPoolTotalCount();
	pnd.PacketUseCount = SerialRingBuffer::GetPoolUseCount();
	pnd.AcceptTPS = InterlockedExchange(&Pnd.AcceptTPS, 0);
	pnd.SendTPS = InterlockedExchange(&Pnd.SendTPS, 0);
	pnd.RecvTPS = InterlockedExchange(&Pnd.RecvTPS, 0);
	return;
}

void CWanServer::GetPHD(PerformanceHardwareData& phd)
{
	Phd.GetPHD(phd);
	return;
}

bool CWanServer::ListenSocketInit(const WCHAR* IP, WORD Port, bool Nagle)
{
	ULONG Ip;
	InetPton(AF_INET, IP, &Ip);

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
	setsockopt(ListenSocket,  //�ش� ����
		IPPROTO_TCP,          //������ ����
		TCP_NODELAY,          //���� �ɼ�
		(const char*)&option, // �ɼ� ������
		sizeof(option));      //�ɼ� ũ��

	// Listen ���� �ּ� �ʱ�ȭ
	SOCKADDR_IN ListenSockAddr;
	ZeroMemory(&ListenSockAddr, sizeof(ListenSockAddr));
	ListenSockAddr.sin_family = AF_INET;
	ListenSockAddr.sin_addr.S_un.S_addr = htonl(Ip);
	ListenSockAddr.sin_port = htons(Port);

	// ���ε�
	if (bind(ListenSocket, (sockaddr*)&ListenSockAddr, sizeof(ListenSockAddr)) == SOCKET_ERROR)
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

void CWanServer::ClientInfo(Session* pSession, WCHAR* IP, WORD* Port)
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

bool CWanServer::ClientInfo(DWORD64 SessionID, WCHAR* IP, WORD* Port)
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

	// Ŭ���̾�Ʈ ���� ���
	SOCKADDR_IN ClientAddr;
	int AddrLen = sizeof(ClientAddr);
	getpeername(pSession->socket, (SOCKADDR*)&ClientAddr, &AddrLen);

	// IP�� port ����
	*Port = ntohs(ClientAddr.sin_port);
	InetNtop(AF_INET, &ClientAddr.sin_addr, IP, 16);

	if (0 == InterlockedDecrement(&pSession->IOCount))
		SessionRelease(pSession);
	return true;
}

void CWanServer::NetworkError(int Err, const WCHAR* FunctionName)
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

void CWanServer::NetworkMessageProc(Session* pSession)
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
		if (NetworkHeader.Code != _PacketCode)
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


			CPacket packet(Packet);
			OnRecv(pSession->SessionID, packet);

			// Monitor ��
			InterlockedIncrement(&Pnd.RecvTPS);
		}
		else break;
	}

	return;
}

void CWanServer::RecvPost(Session* pSession)
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

void CWanServer::SendPost(Session* pSession)
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
			InterlockedExchange(&pSession->SendFlag, 0);
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
	if (1 == InterlockedIncrement(&pSession->IOCount))
	{
		DebugBreak();
		return;
	}

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

void CWanServer::SessionRelease(Session* pSession)
{
	DWORD CompareIOCount = IOCOUNT(0, 0);
	DWORD ExchangeIOCount = IOCOUNT(1, 0);
	DWORD retIOCount = InterlockedCompareExchange(&pSession->IOCount, ExchangeIOCount, CompareIOCount);
	if (CompareIOCount != retIOCount)
		return;

	DWORD64 SessionID = SessionFree(pSession);

	OnSessionRelease(SessionID);

	return;
}

CWanServer::Session* CWanServer::SessionFind(DWORD64 SessionID)
{
	return &Sessions[SESSIONID_INDEX_LOW(SessionID)];
}

CWanServer::Session* CWanServer::SessionAlloc(SOCKET Socket, DWORD64 sessionID)
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
	ZeroMemory(&Sessions[Index].RecvOverlapped, sizeof(OVERLAPPED));
	ZeroMemory(&Sessions[Index].SendOverlapped, sizeof(OVERLAPPED));
	InterlockedIncrement(&Sessions[Index].IOCount);
	InterlockedAnd((long*)&Sessions[Index].IOCount, 0x0000'ffff);

	return &Sessions[Index];
}

DWORD64 CWanServer::SessionFree(Session* pSession)
{
	closesocket(pSession->socket);

	DWORD64 SessionID = pSession->SessionID;
	DWORD Index = SESSIONID_INDEX_LOW(pSession->SessionID);
	DWORD sessionID = SESSIONID_ID_HIGH(pSession->SessionID);
	pSession->SessionID = INVALID_SESSIONID;
	pSession->socket = INVALID_SOCKET;


	// SendQ�� SerialRingBuffer RefCount Sub �� SendQ���� �����
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
		if (iCnt_2 < 0)
			DebugBreak();

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

	EnterCriticalSection(&CS_SessionIndex);
	SessionIndex.push(Index);
	LeaveCriticalSection(&CS_SessionIndex);

	return SessionID;
}

void CWanServer::OutputDebugStringF(LPCTSTR format, ...)
{
	WCHAR buffer[1000] = { 0 };

	va_list arg; // �����μ� ����

	va_start(arg, format);
	vswprintf_s(buffer, 1000, format, arg);
	va_end(arg);

	OutputDebugString(buffer);
	return;
}

void CWanServer::Crash()
{
	// �� �ڵ�� null �����͸� �������Ͽ� ũ���ø� �߻���ŵ�ϴ�.
	int* Nint = NULL;
	*Nint = 0;
}

void CWanServer::DisconnectSession(LPVOID p)
{
	CWanServer* This = (CWanServer*)p;
	
	for (int iCnt = 0; iCnt < This->MaxSessionCount; ++iCnt)
	{
		DWORD64 SessionID = This->DisconnectedSessions[iCnt].SessionID;
		if (SessionID != INVALID_SESSIONID)
		{
			DWORD64 CurrentTick = GetTickCount64();
			if(CurrentTick - This->DisconnectedSessions[iCnt].Tick > This->DisconnectedSessions[iCnt].dwSeconds * 1000)
			{
				This->Disconnect(SessionID);
				InterlockedCompareExchange64((LONG64*)&This->DisconnectedSessions[iCnt].SessionID, INVALID_SESSIONID, SessionID);
			}
		}
	}

	for (int iCnt = 0; iCnt < This->MaxSessionCount; ++iCnt)
	{
		Session* pSession = &This->Sessions[iCnt];
		DWORD64 SessionID = This->Sessions[iCnt].SessionID;
		if (SessionID != INVALID_SESSIONID)
		{
			int a = 1;
		}
	}

	return;
}

void CWanServer::RegisterTimer(void (*TimerFunction)(LPVOID), LPVOID p, DWORD dwMilliseconds)
{
	if (InterlockedAdd(&TimerCount, 0) == MAXIMUM_WAIT_OBJECTS)
	{
		LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"TimerList Lack");
		Crash();
	}

	hTimers[TimerCount] = CreateWaitableTimer(NULL, FALSE, NULL);
	if (hTimers[TimerCount] == NULL)
	{
		int err = GetLastError();
		LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"Failed CreateWaitableTimer() ErrorCode : %d", err);
		Crash();
	}

	TimerList[TimerCount].TimerFunction = TimerFunction;
	TimerList[TimerCount].Time.QuadPart = (dwMilliseconds * -10000LL);
	TimerList[TimerCount].p = p;


	InterlockedIncrement(&TimerCount);
	return;
}

unsigned WINAPI CWanServer::WorkerThreadWrapping(LPVOID p)
{
	CWanServer* This = (CWanServer*)p;
	This->WorkerThread();

	return 0;
}
void CWanServer::WorkerThread()
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
			PostQueuedCompletionStatus(hCP, -1, 0, 0);
			break;
		}


		// PQCS ���� (cbTransferred�� �׻� -1���� �ΰ� Key(pSession), Ovelapped�� �ٲ۴� ex) PQCS(hcp, -1,0,0))
		if (retval == true && cbTransferred == -1)
		{
			// WorkerThread ����
			if (Overlapped == (OVERLAPPED*)0)
			{
				PostQueuedCompletionStatus(hCP, -1, 0, 0);
				break;
			}

			// SendPost
			if (Overlapped == (OVERLAPPED*)1)
			{
				// WSASend
				SendPost(pSession);

				if (0 == InterlockedDecrement(&pSession->IOCount))
					SessionRelease(pSession);

				continue;
			}

			// ����ġ ���� PQCS�� ���� ���
			LOG(L"Network_WanServer", CLog::LEVEL_ERROR,
				L"Unexpected PQCS [retGQCS = %d] [cbTransferred = %ld] [pSession = %lld] [Overlapped = %lld]"
				, retval, cbTransferred, pSession, Overlapped);
			Crash();
		}

		// ���� ���� : Ŭ�󿡼� RST(������ I/O�� ���� �Ϸ������� ó���մϴ�.) or FIN ���� ��� 
		if (cbTransferred == 0)
		{
			int Err = GetLastError();
			NetworkError(Err, L"Asynchronous I/O");
			if (Err == 997)
			{
				LOG(L"Network_WanServer", CLog::LEVEL_DEBUG,
					L"[Error Code : 997] [retGQCS : %d] [cbTransferred : %lld] [Key : %lld] [Overlapped = %lld]",
					retval, cbTransferred, pSession, Overlapped);
			}

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
			// SendQ�� SerialRingBuffer RefCount Sub �� SendQ���� �����
			int UseSize = pSession->SendQ.GetUseSize();
			unsigned int DirectSize = pSession->SendQ.DirectDequeueSize();
			if (DirectSize / 8 >= pSession->SendBufferCount)
			{
				for (unsigned int iCnt = 0; iCnt < pSession->SendBufferCount; ++iCnt)
				{
					SerialRingBuffer* pPacket = (SerialRingBuffer*)*((INT64*)(pSession->SendQ.front() + iCnt * 8));
					pPacket->SubRef();
				}
			}
			else
			{
				int iCnt_1 = DirectSize / 8;
				int iCnt_2 = pSession->SendBufferCount - iCnt_1 - 1;
				if (iCnt_2 < 0)
					DebugBreak();

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

			pSession->SendQ.MoveFront(pSession->SendBufferCount * 8);
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

	return;
}

unsigned WINAPI CWanServer::TimerThreadWrapping(LPVOID p)
{
	CWanServer* This = (CWanServer*)p;
	This->TimerThread();

	return 0;
}
void CWanServer::TimerThread()
{
	for (int iCnt = 0; iCnt < TimerCount; ++iCnt)
	{
		if (0 == SetWaitableTimer(hTimers[iCnt], &TimerList[iCnt].Time, 0, NULL, NULL, 0))
		{
			int err = GetLastError();
			LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"Failed SetWaitableTimer() ErrorCode : %d", err);
			Crash();
		}
	}

	if (TimerCount == 0)
		return;

	while (1)
	{
		DWORD Index = WaitForMultipleObjects(TimerCount, hTimers, FALSE, INFINITE);
		TimerList[Index].TimerFunction(TimerList[Index].p);
		if (0 == SetWaitableTimer(hTimers[Index], &TimerList[Index].Time, 0, NULL, NULL, 0))
		{
			int err = GetLastError();
			LOG(L"Network_WanServer", CLog::LEVEL_ERROR, L"Failed SetWaitableTimer() ErrorCode : %d", err);
			Crash();
		}
	}

	return;
}

unsigned WINAPI CWanServer::AcceptThreadWrapping(LPVOID p)
{
	CWanServer* This = (CWanServer*)p;
	This->AcceptThread();

	return 0;
}
void CWanServer::AcceptThread()
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
		if(_ZeroCopySend)
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
		OnSessionConnected(pSession->SessionID);

		// ���ϰ� ����� �Ϸ� ��Ʈ ����
		CreateIoCompletionPort((HANDLE)ClientSocket, hCP, (ULONG_PTR)pSession, 0);

		// �񵿱� ����� ����
		RecvPost(pSession);

		// Monitor��
		InterlockedIncrement(&Pnd.AcceptTPS);
	}

	// AcceptThread ����
	LOG(L"Network_WanServer", CLog::LEVEL_SYSTEM, L"AcceptThread Release");

	return;
}