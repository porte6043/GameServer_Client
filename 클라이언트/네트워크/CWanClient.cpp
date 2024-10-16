#include "CWanClient.h"

CWanClient::CWanClient() : IP{}, hTimers{}, TimerList{}
{
	// wprintf�� �ѱ� ���
	_wsetlocale(LC_ALL, L"korean");

	TimerCount = 0;
	Port = 0;
	Nagle = false;
	ZeroCopySend = false;
	AutoConnect = false;
	NumberOfCore = 0;
	hCP = nullptr;
	hWorker_Threads = nullptr;
	hWorker_Threads = nullptr;

}
CWanClient::~CWanClient()
{

}

bool CWanClient::ReConnect()
{
	// Client ���� ����
	SOCKET Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (Socket == INVALID_SOCKET)
	{
		int Err = WSAGetLastError();
		LOG(L"Network_WanClient", CLog::LEVEL_ERROR,
			L"Fail socket() ErrorCode : %d", Err);
		return false;
	}
	LOG(L"Network_WanClient", CLog::LEVEL_SYSTEM, L"ReConnect socket()");

	// ���̱� �˰��� on/off
	int option = Nagle;
	setsockopt(Socket,  //�ش� ����
		IPPROTO_TCP,			//������ ����
		TCP_NODELAY,			//���� �ɼ�
		(const char*)&option,	// �ɼ� ������
		sizeof(option));		//�ɼ� ũ��

	// �񵿱� I/O�� ���� TCP SendBufSize = 0
	if (ZeroCopySend)
	{
		int optval = 0;	// send ���� ������
		setsockopt(Socket, SOL_SOCKET, SO_SNDBUF, (char*)&optval, sizeof(optval));
	}

	// ���ϰ� ����� �Ϸ� ��Ʈ ����
	CreateIoCompletionPort((HANDLE)Socket, hCP, (ULONG_PTR)&_Session, 0);

	// IP ���� ���� ��ȯ
	ULONG Ip;
	InetPton(AF_INET, IP, &Ip);

	// Client ���� �ּ� �ʱ�ȭ
	SOCKADDR_IN ClientSockAddr;
	ZeroMemory(&ClientSockAddr, sizeof(SOCKADDR_IN));
	ClientSockAddr.sin_family = AF_INET;
	//ClientSockAddr.sin_addr.S_un.S_addr = htonl(Ip);
	InetPton(AF_INET, IP, &ClientSockAddr.sin_addr);
	ClientSockAddr.sin_port = htons(Port);

	// connect
	if (connect(Socket, reinterpret_cast<sockaddr*>(&ClientSockAddr), sizeof(ClientSockAddr)) == SOCKET_ERROR)
	{
		int Err = WSAGetLastError();
		LOG(L"Network_WanClient", CLog::LEVEL_ERROR,
			L"Fail connect() ErrorCode : %d", Err);
		closesocket(Socket);
		return false;
	}

	_Session.socket = Socket;
	_Session.SendFlag = 0;
	_Session.DisconnectFlag = 0;
	_Session.SendBufferCount = 0;
	_Session.RecvQ.clear();
	_Session.SendQ.clear();
	ZeroMemory(&_Session.RecvOverlapped, sizeof(OVERLAPPED));
	ZeroMemory(&_Session.SendOverlapped, sizeof(OVERLAPPED));
	InterlockedIncrement(&_Session.IOCount);
	InterlockedAnd((long*)&_Session.IOCount, 0x0000'ffff);

	// ������ ���� ����
	OnConnectServer();

	// �񵿱� ����� ����
	RecvPost();

	return true;
}
bool CWanClient::ReConnect(wstring ip, WORD port)
{
	// Client ���� ����
	SOCKET Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (Socket == INVALID_SOCKET)
	{
		int Err = WSAGetLastError();
		LOG(L"Network_WanClient", CLog::LEVEL_ERROR,
			L"Fail socket() ErrorCode : %d", Err);
		return false;
	}
	LOG(L"Network_WanClient", CLog::LEVEL_SYSTEM, L"ReConnect socket()");

	// ���̱� �˰��� on/off
	int option = Nagle;
	setsockopt(Socket,  //�ش� ����
		IPPROTO_TCP,			//������ ����
		TCP_NODELAY,			//���� �ɼ�
		(const char*)&option,	// �ɼ� ������
		sizeof(option));		//�ɼ� ũ��

	// �񵿱� I/O�� ���� TCP SendBufSize = 0
	if (ZeroCopySend)
	{
		int optval = 0;	// send ���� ������
		setsockopt(Socket, SOL_SOCKET, SO_SNDBUF, (char*)&optval, sizeof(optval));
	}

	// ���ϰ� ����� �Ϸ� ��Ʈ ����
	CreateIoCompletionPort((HANDLE)Socket, hCP, (ULONG_PTR)&_Session, 0);

	// ���� ip, port ����
	wcsncpy_s(IP, ip.c_str(), 16);
	IP[15] = L'\0';
	Port = port;
	// IP ���� ���� ��ȯ
	ULONG Ip;
	InetPton(AF_INET, IP, &Ip);

	// Client ���� �ּ� �ʱ�ȭ
	SOCKADDR_IN ClientSockAddr;
	ZeroMemory(&ClientSockAddr, sizeof(SOCKADDR_IN));
	ClientSockAddr.sin_family = AF_INET;
	//ClientSockAddr.sin_addr.S_un.S_addr = htonl(Ip);
	InetPton(AF_INET, IP, &ClientSockAddr.sin_addr);
	ClientSockAddr.sin_port = htons(Port);

	// connect
	if (connect(Socket, reinterpret_cast<sockaddr*>(&ClientSockAddr), sizeof(ClientSockAddr)) == SOCKET_ERROR)
	{
		int Err = WSAGetLastError();
		LOG(L"Network_WanClient", CLog::LEVEL_ERROR,
			L"Fail connect() ErrorCode : %d", Err);
		closesocket(Socket);
		return false;
	}

	_Session.socket = Socket;
	_Session.SendFlag = 0;
	_Session.DisconnectFlag = 0;
	_Session.SendBufferCount = 0;
	_Session.RecvQ.clear();
	_Session.SendQ.clear();
	ZeroMemory(&_Session.RecvOverlapped, sizeof(OVERLAPPED));
	ZeroMemory(&_Session.SendOverlapped, sizeof(OVERLAPPED));
	InterlockedIncrement(&_Session.IOCount);
	InterlockedAnd((long*)&_Session.IOCount, 0x0000'ffff);
	 
	// ������ ���� ����
	OnConnectServer();

	// �񵿱� ����� ����
	RecvPost();

	return true;
}
bool CWanClient::Connect(const wchar_t* ip, WORD port, WORD numberOfCreateThreads, WORD numberOfConcurrentThreads, bool nagle, bool zeroCopySend, bool autoConnect, BYTE packetCode, BYTE packetKey)
{
	// ��� ���� Set
	wmemcpy_s(IP, 16, ip, wcslen(ip));
	Port = port;
	Nagle = nagle;
	ZeroCopySend = zeroCopySend;
	NumberOfCore = numberOfCreateThreads;
	AutoConnect = autoConnect;
	_PacketCode = packetCode;
	_PacketKey = packetKey;

	// ��Ʈ��ũ ��� PacketKey Set
	SerialRingBuffer::SetPacketKey(_PacketKey);

	// ���� �ʱ�ȭ
	WSADATA WSA;
	if (WSAStartup(MAKEWORD(2, 2), &WSA) != 0)
	{
		int Err = WSAGetLastError();
		LOG(L"Network_WanClient", CLog::LEVEL_ERROR,
			L"Fail WSAStartup() ErrorCode : %d", Err);
		return false;
	}
	LOG(L"Network_WanClient", CLog::LEVEL_SYSTEM, L"WSAStartup()");

	// ����� �Ϸ� ��Ʈ ����
	hCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, numberOfConcurrentThreads);
	if (hCP == NULL)
	{
		int Err = GetLastError();
		LOG(L"Network_WanClient", CLog::LEVEL_ERROR, L"Fail CreateIoCompletionPort() ErrorCode : %d", Err);
		return 1;
	}

	// IOCP ��Ŀ������ ����
	hWorker_Threads = new HANDLE[NumberOfCore];
	for (unsigned int iCnt = 0; iCnt < NumberOfCore; ++iCnt)
	{
		hWorker_Threads[iCnt] = (HANDLE)_beginthreadex(nullptr, 0, WorkerThreadWrapping, this, 0, nullptr);
		if (hWorker_Threads[iCnt] == 0)
		{
			int Err = errno;
			LOG(L"Network_WanClient", CLog::LEVEL_ERROR, L"Fail _beginthreadex() ErrorCode : %d", Err);
			return false;
		}
	}

	// Ÿ�̸� ������ ����
	hTimer_Threads = (HANDLE)_beginthreadex(nullptr, 0, TimerThreadWrapping, this, 0, nullptr);
	if (hTimer_Threads == 0)
	{
		int Err = errno;
		LOG(L"Network_WanClient", CLog::LEVEL_ERROR, L"Fail _beginthreadex() ErrorCode : %d", Err);
		return false;
	}

	// Client ���� ����
	_Session.socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_Session.socket == INVALID_SOCKET)
	{
		int Err = WSAGetLastError();
		LOG(L"Network_WanClient", CLog::LEVEL_ERROR,
			L"Fail socket() ErrorCode : %d", Err);
		return false;
	}
	LOG(L"Network_WanClient", CLog::LEVEL_SYSTEM, L"socket()");

	// ���̱� �˰��� on/off
	int option = Nagle;
	setsockopt(_Session.socket,  //�ش� ����
		IPPROTO_TCP,			//������ ����
		TCP_NODELAY,			//���� �ɼ�
		(const char*)&option,	// �ɼ� ������
		sizeof(option));		//�ɼ� ũ��

	// �񵿱� I/O�� ���� TCP SendBufSize = 0
	if (ZeroCopySend)
	{
		int optval = 0;	// send ���� ������
		setsockopt(_Session.socket, SOL_SOCKET, SO_SNDBUF, (char*)&optval, sizeof(optval));
	}

	// ���ϰ� ����� �Ϸ� ��Ʈ ����
	CreateIoCompletionPort((HANDLE)_Session.socket, hCP, (ULONG_PTR)&_Session, 0);

	// IP ���� ���� ��ȯ
	ULONG Ip;
	InetPton(AF_INET, IP, &Ip);

	// Client ���� �ּ� �ʱ�ȭ
	SOCKADDR_IN ClientSockAddr;
	ZeroMemory(&ClientSockAddr, sizeof(SOCKADDR_IN));
	ClientSockAddr.sin_family = AF_INET;
	//ClientSockAddr.sin_addr.S_un.S_addr = htonl(Ip);
	InetPton(AF_INET, IP, &ClientSockAddr.sin_addr);
	ClientSockAddr.sin_port = htons(Port);

	// connect
	if (connect(_Session.socket, reinterpret_cast<sockaddr*>(&ClientSockAddr), sizeof(ClientSockAddr)) == SOCKET_ERROR)
	{
		int Err = WSAGetLastError();
		LOG(L"Network_WanClient", CLog::LEVEL_ERROR,
			L"Fail connect() ErrorCode : %d", Err);
		PostQueuedCompletionStatus(hCP, -1, 0, (OVERLAPPED*)2);
		return false;
	}

	// ������ ���� ����
	OnConnectServer();

	// �񵿱� ����� ����
	RecvPost();

	return true;
}
bool CWanClient::Disconnect()
{
	int retIOCount = InterlockedIncrement(&_Session.IOCount);

	if (IOCOUNT_FLAG_HIGH(retIOCount) == 1)
	{
		if (0 == InterlockedDecrement(&_Session.IOCount))
			SessionRelease();
		return false;
	}

	InterlockedExchange(&_Session.DisconnectFlag, 1);
	CancelIoEx(reinterpret_cast<HANDLE>(_Session.socket), NULL);

	if (0 == InterlockedDecrement(&_Session.IOCount))
		SessionRelease();

	return true;
}

bool CWanClient::SendMSG(CPacket& packet)
{
	int retIOCount = InterlockedIncrement(&_Session.IOCount);
	if (IOCOUNT_FLAG_HIGH(retIOCount) == 1)
	{
		if (0 == InterlockedDecrement(&_Session.IOCount))
			SessionRelease();
		return false;
	}

	if (_Session.DisconnectFlag == 1)
	{
		if (0 == InterlockedDecrement(&_Session.IOCount))
			SessionRelease();
		return false;
	}

	SerialRingBuffer* Packet = packet.Packet;
	NetworkHeader NetworkHeader(_PacketCode, Packet);
	Packet->SetWanHeader(&NetworkHeader, sizeof(NetworkHeader));
	Packet->encode();

	Packet->AddRef();
	EnterCriticalSection(&_Session.CS_SendQ);
	_Session.SendQ.enqueue(&Packet, sizeof(SerialRingBuffer*));
	LeaveCriticalSection(&_Session.CS_SendQ);

	// WSASend
	SendPost();

	if (0 == InterlockedDecrement(&_Session.IOCount))
		SessionRelease();

	// Monitor��
	InterlockedIncrement(&Pnd.SendTPS);
	return true;
}
bool CWanClient::SendMSG_PQCS(CPacket& packet)
{
	int retIOCount = InterlockedIncrement(&_Session.IOCount);
	if (IOCOUNT_FLAG_HIGH(retIOCount) == 1)
	{
		if (0 == InterlockedDecrement(&_Session.IOCount))
			SessionRelease();
		return false;
	}

	if (_Session.DisconnectFlag == 1)
	{
		if (0 == InterlockedDecrement(&_Session.IOCount))
			SessionRelease();
		return false;
	}

	SerialRingBuffer* Packet = packet.Packet;
	NetworkHeader NetworkHeader(_PacketCode, Packet);
	Packet->SetWanHeader(&NetworkHeader, sizeof(NetworkHeader));
	Packet->encode();

	Packet->AddRef();
	EnterCriticalSection(&_Session.CS_SendQ);
	_Session.SendQ.enqueue(&Packet, sizeof(SerialRingBuffer*));
	LeaveCriticalSection(&_Session.CS_SendQ);

	PostQueuedCompletionStatus(hCP, -1, (ULONG_PTR)&_Session, (LPOVERLAPPED)1);

	// Monitor��
	InterlockedIncrement(&Pnd.SendTPS);
	return true;
}


void CWanClient::GetPND(PerformanceNetworkData& pnd)
{
	pnd.PacketTotalCount = SerialRingBuffer::GetPoolTotalCount();
	pnd.PacketUseCount = SerialRingBuffer::GetPoolUseCount();
	pnd.SendTPS = InterlockedExchange(&Pnd.SendTPS, 0);
	pnd.RecvTPS = InterlockedExchange(&Pnd.RecvTPS, 0);
	return;
}
void CWanClient::GetPHD(PerformanceHardwareData& phd)
{
	Phd.GetPHD(phd);
	return;
}

// ��Ʈ��ũ�� ������ �߻��� �� �α��� ���� �Լ��Դϴ�.
void CWanClient::NetworkError(int Err, const WCHAR* FunctionName)
{
	//Err == WSAEINVAL						// 10022 : WSASend���� dwBufferCount �μ��� '0'�� ���� �� ��Ÿ���� ���� Ȯ��

	if (Err != WSAECONNRESET &&				// 10054 : Ŭ�� ������ ������ ���
		Err != WSAECONNABORTED &&			// 10053 : ����Ʈ����(TCP)�� ������ �ߴܵ� ���
		Err != ERROR_NETNAME_DELETED &&		// 64	 : RST, Ŭ�󿡼� ���� ���� ���
		Err != WSAENOTSOCK &&				// 10038 : ������ �ƴ� �׸񿡼� �۾��� �õ��߽��ϴ�.
		Err != ERROR_OPERATION_ABORTED &&	// 995	 : ������ ���� �Ǵ� ���ø����̼� ��û���� ���� I/O �۾��� �ߴܵǾ����ϴ�. (CancelIoEx)
		Err != ERROR_SUCCESS)				// 0	 : �۾��� ���������� �Ϸ�Ǿ����ϴ�. (FIN�� �޾��� ���)
	{
		LOG(L"Network_WanClient", CLog::LEVEL_ERROR, L"Fail %s ErrorCode : %d", FunctionName, Err);
	}
	return;
}

// ��Ʈ��ũ �޽����� ��� Ȯ���ؼ� ������ Paload ���� �� �����մϴ�.
void CWanClient::NetworkMessageProc()
{
	while (1)
	{
		// NetworkHeader ���� ũ�� �ִ��� Ȯ��
		if (sizeof(NetworkHeader) >= _Session.RecvQ.GetUseSize())
			break;

		// Network Header peek
		NetworkHeader NetworkHeader;
		int retpeek = _Session.RecvQ.peek(&NetworkHeader, sizeof(NetworkHeader));

		// PayLoad�� ���Դ��� Ȯ��
		if (_Session.RecvQ.GetUseSize() - sizeof(NetworkHeader) >= NetworkHeader.Len)
		{
			// Network ��� ũ�⸸ŭ �̵�
			_Session.RecvQ.MoveFront(sizeof(NetworkHeader));

			SerialRingBuffer* Packet = SerialRingBuffer::Alloc();
			Packet->SetWanHeader(&NetworkHeader, sizeof(NetworkHeader));
			int retDeq = _Session.RecvQ.dequeue(Packet->buffer(), NetworkHeader.Len);
			if (retDeq != NetworkHeader.Len)
			{
				LOG(L"Network_WanClient", CLog::LEVEL_ERROR, L"Failed dequeue()");
				Crash();
			}
			Packet->MoveRear(retDeq);

			// decode
			if (!Packet->decode())
			{
				// decode ����
				LOG(L"Network_WanServer", CLog::LEVEL_DEBUG, L"Failed decode()");
				Packet->Free();
				Disconnect();
				break;
			}

			CPacket packet(Packet);
			OnRecv(packet);

			// Monitor ��
			InterlockedIncrement(&Pnd.RecvTPS);
		}
		else break;
	}

	return;
}

// WSARecv ������
void CWanClient::RecvPost()
{
	// �񵿱� ����� ����
	DWORD Flags = 0;
	WSABUF WsaBuf[2];
	WsaBuf[0].buf = _Session.RecvQ.rear();
	WsaBuf[0].len = _Session.RecvQ.DirectEnqueueSize();
	WsaBuf[1].buf = _Session.RecvQ.buffer();
	WsaBuf[1].len = _Session.RecvQ.GetFreeSize() - _Session.RecvQ.DirectEnqueueSize();
	ZeroMemory(&_Session.RecvOverlapped, sizeof(OVERLAPPED));

	InterlockedIncrement(&_Session.IOCount);
	int retRecv = WSARecv(_Session.socket, WsaBuf, 2, nullptr, &Flags, (OVERLAPPED*)&_Session.RecvOverlapped, NULL);
	if (retRecv == SOCKET_ERROR)
	{
		int Err = WSAGetLastError();
		if (Err != ERROR_IO_PENDING)
		{
			NetworkError(Err, L"WSARecv");
			// WSARecv�� ���� ���� ��� IOCount ����
			if (0 == InterlockedDecrement(&_Session.IOCount))
				SessionRelease();
		}
		else
		{
			if (_Session.DisconnectFlag == 1)
				CancelIoEx(reinterpret_cast<HANDLE>(_Session.socket), NULL);
		}
	}

	if (0 == InterlockedDecrement(&_Session.IOCount))
		SessionRelease();
	return;
}

//  WSASend ������
void CWanClient::SendPost()
{
	/*
	Send�� 1ȸ ����
	1ȸ�� ���� ���� ������ WSASend �Ϸ������� �ͼ� �������� front�� ó�� �Ǳ� ���� WSARecv �Ϸ�������
	���� �ͼ� �޽��� ó�� �� WSARecv�Ϸ����� �ʿ��� �ٽ� �ѹ� WSASend�� �ϰ� �Ǹ� ���� WSASend�� front��
	ó�� �Ǳ� ���� Send�� �ϰ� �Ǿ� �޽����� ��ø�Ȱ� ������ �� �ִ�.
	*/
	SerialRingBuffer* SendQ = &_Session.SendQ;

	/*
		���� ���������� UseSize�� ������� ������, SendMSG�� ���� SendPost�� ȣ�� �Ǹ� Session�� lock�� �ɷ��־ ��� ������
		WorkerThread���� Send�Ϸ��������� SendPost�� ȣ�� �ϴ� ��� Session�� lock�� �ɷ����� �ʱ� ������ ��� ���ο��� GetUs
		esize()�� ũ�Ⱑ �޶��� �� �ִ�.
	*/
	while (1)
	{
		if (1 == InterlockedExchange(&_Session.SendFlag, 1))
			return;

		if (SendQ->GetUseSize() != 0)
		{
			break;
		}
		else
		{
			InterlockedExchange(&_Session.SendFlag, 0);
			if (SendQ->GetUseSize() != 0)
				continue;
			else
				return;
		}
	}

	// SerialRingBuffer�� point�� SendQ�� �ֽ��ϴ�.
	WSABUF WsaBuf[800];
	_Session.SendBufferCount = SendQ->GetUseSize() / 8;
	unsigned int DirectSize = _Session.SendQ.DirectDequeueSize();
	if (DirectSize / 8 >= _Session.SendBufferCount)
	{
		for (unsigned int iCnt = 0; iCnt < _Session.SendBufferCount; ++iCnt)
		{
			SerialRingBuffer* pPacket = (SerialRingBuffer*)*((INT64*)(SendQ->front() + iCnt * 8));

			WsaBuf[iCnt].buf = pPacket->WanHead();
			WsaBuf[iCnt].len = pPacket->GetUseSize() + sizeof(NetworkHeader);
		}
		ZeroMemory(&_Session.SendOverlapped, sizeof(OVERLAPPED));
	}
	else
	{
		int index = 0;
		int iCnt_1 = DirectSize / 8;
		int iCnt_2 = _Session.SendBufferCount - iCnt_1 - 1;

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

		ZeroMemory(&_Session.SendOverlapped, sizeof(OVERLAPPED));
	}

	InterlockedIncrement(&_Session.IOCount);
	int retSend = WSASend(_Session.socket, WsaBuf, _Session.SendBufferCount, nullptr, 0, (OVERLAPPED*)&_Session.SendOverlapped, NULL);
	if (retSend == SOCKET_ERROR)
	{
		int Err = WSAGetLastError();
		if (Err != WSA_IO_PENDING)
		{
			NetworkError(Err, L"WSASend");

			if (0 == InterlockedDecrement(&_Session.IOCount))
				SessionRelease();
		}
	}

	return;
}

// Session ����
void CWanClient::SessionRelease()
{
	DWORD CompareIOCount = IOCOUNT(0, 0);
	DWORD ExchangeIOCount = IOCOUNT(1, 0);
	DWORD retIOCount = InterlockedCompareExchange(&_Session.IOCount, ExchangeIOCount, CompareIOCount);
	if (CompareIOCount != retIOCount)
		return;

	closesocket(_Session.socket);
	_Session.socket = INVALID_SOCKET;


	// SendQ�� SerialRingBuffer RefCount Sub �� SendQ���� �����
	int UseSize = _Session.SendQ.GetUseSize();
	unsigned int DirectSize = _Session.SendQ.DirectDequeueSize();
	int SubCount = UseSize / 8;
	if (DirectSize / 8 >= SubCount)
	{
		for (unsigned int iCnt = 0; iCnt < SubCount; ++iCnt)
		{
			SerialRingBuffer* pPacket = (SerialRingBuffer*)*((INT64*)(_Session.SendQ.front() + iCnt * 8));
			pPacket->SubRef();
		}
	}
	else
	{
		int iCnt_1 = DirectSize / 8;
		int iCnt_2 = SubCount - iCnt_1 - 1;

		for (int iCnt = 0; iCnt < iCnt_1; ++iCnt)
		{
			SerialRingBuffer* pPacket = (SerialRingBuffer*)*((INT64*)(_Session.SendQ.front() + iCnt * 8));
			pPacket->SubRef();
		}
		char Buffer[8];

		memcpy(Buffer, _Session.SendQ.front() + iCnt_1 * 8, DirectSize - iCnt_1 * 8);
		memcpy(Buffer + DirectSize - iCnt_1 * 8, _Session.SendQ.buffer(), 8 - (DirectSize - iCnt_1 * 8));
		SerialRingBuffer* pPacket = (SerialRingBuffer*)*((INT64*)Buffer);
		pPacket->SubRef();

		char* front = _Session.SendQ.buffer() + 8 - (DirectSize - iCnt_1 * 8);
		for (int iCnt = 0; iCnt < iCnt_2; ++iCnt)
		{
			SerialRingBuffer* pPacket = (SerialRingBuffer*)*((INT64*)(front + iCnt * 8));
			pPacket->SubRef();
		}
	}

	OnDisconnectServer();
	if (AutoConnect)
	{
		PostQueuedCompletionStatus(hCP, -1, 0, (OVERLAPPED*)2);
	}

	return;
}

// ���â�� ����մϴ�.
void CWanClient::OutputDebugStringF(LPCTSTR format, ...)
{
	WCHAR buffer[1000] = { 0 };

	va_list arg; // �����μ� ����

	va_start(arg, format);
	vswprintf_s(buffer, 1000, format, arg);
	va_end(arg);

	OutputDebugString(buffer);
	return;
}

// �Լ� ���ο��� �Ϻη� ũ������ ���ϴ�. �߻� �ϸ� �ڵ��� �����Դϴ�.
void CWanClient::Crash()
{
	// �� �ڵ�� null �����͸� �������Ͽ� ũ���ø� �߻���ŵ�ϴ�.
	int* Nint = NULL;
	*Nint = 0;
}

void CWanClient::RegisterTimer(void (*TimerFunction)(LPVOID), LPVOID p, DWORD dwMilliseconds)
{
	if (InterlockedAdd(&TimerCount, 0) == MAXIMUM_WAIT_OBJECTS)
	{
		LOG(L"Network_WanClient", CLog::LEVEL_ERROR, L"TimerList Lack");
		Crash();
	}

	hTimers[TimerCount] = CreateWaitableTimer(NULL, FALSE, NULL);
	if (hTimers[TimerCount] == NULL)
	{
		int err = GetLastError();
		LOG(L"Network_WanClient", CLog::LEVEL_ERROR, L"Failed CreateWaitableTimer() ErrorCode : %d", err);
		Crash();
	}

	TimerList[TimerCount].TimerFunction = TimerFunction;
	TimerList[TimerCount].Time.QuadPart = (dwMilliseconds * -10000LL);
	TimerList[TimerCount].p = p;


	InterlockedIncrement(&TimerCount);
	return;
}

unsigned WINAPI CWanClient::WorkerThreadWrapping(LPVOID p)
{
	CWanClient* This = (CWanClient*)p;
	This->WorkerThread();

	return 0;
}
void CWanClient::WorkerThread()
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
			LOG(L"Network_WanClient", CLog::LEVEL_SYSTEM, L"Failed GQCS ErrorCode : %d", Err);
			PostQueuedCompletionStatus(hCP, -1, 0, 0);
			break;
		}

		// PQCS ���� (cbTransferred�� �׻� -1���� �ΰ� Key(pSession), Ovelapped�� �ٲ۴� ex) -1, 0, 0)
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
				SendPost();

				if (0 == InterlockedDecrement(&_Session.IOCount))
					SessionRelease();

				continue;
			}

			// AutoConnect
			if (Overlapped == (OVERLAPPED*)2)
			{
				if (!ReConnect())
				{
					Sleep(3000);
					PostQueuedCompletionStatus(hCP, -1, 0, (OVERLAPPED*)2);
				}
				continue;
			}

			// ����ġ ���� PQCS�� ���� ���
			LOG(L"Network_WanClient", CLog::LEVEL_ERROR,
				L"Unexpected PQCS [retGQCS = %d] [cbTransferred = %ld] [Overlapped = %lld]"
				, retval, cbTransferred, Overlapped);
			Crash();
		}

		// ���� ���� : Ŭ�󿡼� RST ���� ��� (������ I/O�� ���� �Ϸ������� ó���մϴ�.)
		if (cbTransferred == 0)
		{
			int Err = GetLastError();
			NetworkError(Err, L"Asynchronous I/O");
			if (Err == 997)
			{
				LOG(L"Network_WanClient", CLog::LEVEL_DEBUG,
					L"[Error Code : 997] [retGQCS : %d] [cbTransferred : %lld] [Key : %lld] [Overlapped = %lld]",
					retval, cbTransferred, pSession, Overlapped);
			}

			Disconnect();

			// ������ �Ϸ������� ���� IOCount ���ҽ�ŵ�ϴ�.
			if (0 == InterlockedDecrement(&_Session.IOCount))
				SessionRelease();
			continue;
		}

		// WSARecv �Ϸ�   
		if (Overlapped == &_Session.RecvOverlapped)
		{
			// RecvQ rear �̵�
			_Session.RecvQ.MoveRear(cbTransferred);

			// �޽��� ó��
			NetworkMessageProc();

			// Recv �ٽ� �ɾ� �Ӵϴ�.
			RecvPost();
			continue;
		}

		// WSASend �Ϸ�
		if (Overlapped == &_Session.SendOverlapped)
		{
			// SendQ�� SerialRingBuffer RefCount Sub �� SendQ���� �����
			int UseSize = _Session.SendQ.GetUseSize();
			unsigned int DirectSize = _Session.SendQ.DirectDequeueSize();
			if (DirectSize / 8 >= _Session.SendBufferCount)
			{
				for (unsigned int iCnt = 0; iCnt < _Session.SendBufferCount; ++iCnt)
				{
					SerialRingBuffer* pPacket = (SerialRingBuffer*)*((INT64*)(_Session.SendQ.front() + iCnt * 8));
					pPacket->SubRef();
				}
			}
			else
			{
				int iCnt_1 = DirectSize / 8;
				int iCnt_2 = _Session.SendBufferCount - iCnt_1 - 1;

				for (int iCnt = 0; iCnt < iCnt_1; ++iCnt)
				{
					SerialRingBuffer* pPacket = (SerialRingBuffer*)*((INT64*)(_Session.SendQ.front() + iCnt * 8));
					pPacket->SubRef();
				}
				char Buffer[8];

				memcpy(Buffer, _Session.SendQ.front() + iCnt_1 * 8, DirectSize - iCnt_1 * 8);
				memcpy(Buffer + DirectSize - iCnt_1 * 8, _Session.SendQ.buffer(), 8 - (DirectSize - iCnt_1 * 8));
				SerialRingBuffer* pPacket = (SerialRingBuffer*)*((INT64*)Buffer);
				pPacket->SubRef();

				char* front = _Session.SendQ.buffer() + 8 - (DirectSize - iCnt_1 * 8);
				for (int iCnt = 0; iCnt < iCnt_2; ++iCnt)
				{
					SerialRingBuffer* pPacket = (SerialRingBuffer*)*((INT64*)(front + iCnt * 8));
					pPacket->SubRef();
				}
			}

			_Session.SendQ.MoveFront(_Session.SendBufferCount * 8);
			_Session.SendBufferCount = 0;

			// SendFlag ����
			InterlockedExchange(&_Session.SendFlag, 0);
			if (0 != _Session.SendQ.GetUseSize())
				SendPost();

			// ������ �Ϸ������� ���� IOCount ���ҽ�ŵ�ϴ�.
			if (0 == InterlockedDecrement(&_Session.IOCount))
				SessionRelease();
		}
	}

	return;
}

unsigned WINAPI CWanClient::TimerThreadWrapping(LPVOID p)
{
	CWanClient* This = (CWanClient*)p;
	This->TimerThread();

	return 0;
}
void CWanClient::TimerThread()
{
	for (int iCnt = 0; iCnt < TimerCount; ++iCnt)
	{
		if (0 == SetWaitableTimer(hTimers[iCnt], &TimerList[iCnt].Time, 0, NULL, NULL, 0))
		{
			int err = GetLastError();
			LOG(L"Network_WanClient", CLog::LEVEL_ERROR, L"Failed SetWaitableTimer() ErrorCode : %d", err);
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
			LOG(L"Network_WanClient", CLog::LEVEL_ERROR, L"Failed SetWaitableTimer() ErrorCode : %d", err);
			Crash();
		}
	}

	return;
}