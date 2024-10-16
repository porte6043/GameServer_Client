#include "CWanClient.h"

CWanClient::CWanClient() : IP{}, hTimers{}, TimerList{}
{
	// wprintf의 한글 사용
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
	// Client 소켓 생성
	SOCKET Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (Socket == INVALID_SOCKET)
	{
		int Err = WSAGetLastError();
		LOG(L"Network_WanClient", CLog::LEVEL_ERROR,
			L"Fail socket() ErrorCode : %d", Err);
		return false;
	}
	LOG(L"Network_WanClient", CLog::LEVEL_SYSTEM, L"ReConnect socket()");

	// 네이글 알고리즘 on/off
	int option = Nagle;
	setsockopt(Socket,  //해당 소켓
		IPPROTO_TCP,			//소켓의 레벨
		TCP_NODELAY,			//설정 옵션
		(const char*)&option,	// 옵션 포인터
		sizeof(option));		//옵션 크기

	// 비동기 I/O를 위한 TCP SendBufSize = 0
	if (ZeroCopySend)
	{
		int optval = 0;	// send 버퍼 사이즈
		setsockopt(Socket, SOL_SOCKET, SO_SNDBUF, (char*)&optval, sizeof(optval));
	}

	// 소켓과 입출력 완료 포트 연결
	CreateIoCompletionPort((HANDLE)Socket, hCP, (ULONG_PTR)&_Session, 0);

	// IP 이진 형태 변환
	ULONG Ip;
	InetPton(AF_INET, IP, &Ip);

	// Client 소켓 주소 초기화
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

	// 서버와 연결 성공
	OnConnectServer();

	// 비동기 입출력 시작
	RecvPost();

	return true;
}
bool CWanClient::ReConnect(wstring ip, WORD port)
{
	// Client 소켓 생성
	SOCKET Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (Socket == INVALID_SOCKET)
	{
		int Err = WSAGetLastError();
		LOG(L"Network_WanClient", CLog::LEVEL_ERROR,
			L"Fail socket() ErrorCode : %d", Err);
		return false;
	}
	LOG(L"Network_WanClient", CLog::LEVEL_SYSTEM, L"ReConnect socket()");

	// 네이글 알고리즘 on/off
	int option = Nagle;
	setsockopt(Socket,  //해당 소켓
		IPPROTO_TCP,			//소켓의 레벨
		TCP_NODELAY,			//설정 옵션
		(const char*)&option,	// 옵션 포인터
		sizeof(option));		//옵션 크기

	// 비동기 I/O를 위한 TCP SendBufSize = 0
	if (ZeroCopySend)
	{
		int optval = 0;	// send 버퍼 사이즈
		setsockopt(Socket, SOL_SOCKET, SO_SNDBUF, (char*)&optval, sizeof(optval));
	}

	// 소켓과 입출력 완료 포트 연결
	CreateIoCompletionPort((HANDLE)Socket, hCP, (ULONG_PTR)&_Session, 0);

	// 접속 ip, port 변경
	wcsncpy_s(IP, ip.c_str(), 16);
	IP[15] = L'\0';
	Port = port;
	// IP 이진 형태 변환
	ULONG Ip;
	InetPton(AF_INET, IP, &Ip);

	// Client 소켓 주소 초기화
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
	 
	// 서버와 연결 성공
	OnConnectServer();

	// 비동기 입출력 시작
	RecvPost();

	return true;
}
bool CWanClient::Connect(const wchar_t* ip, WORD port, WORD numberOfCreateThreads, WORD numberOfConcurrentThreads, bool nagle, bool zeroCopySend, bool autoConnect, BYTE packetCode, BYTE packetKey)
{
	// 멤버 변수 Set
	wmemcpy_s(IP, 16, ip, wcslen(ip));
	Port = port;
	Nagle = nagle;
	ZeroCopySend = zeroCopySend;
	NumberOfCore = numberOfCreateThreads;
	AutoConnect = autoConnect;
	_PacketCode = packetCode;
	_PacketKey = packetKey;

	// 네트워크 헤드 PacketKey Set
	SerialRingBuffer::SetPacketKey(_PacketKey);

	// 윈속 초기화
	WSADATA WSA;
	if (WSAStartup(MAKEWORD(2, 2), &WSA) != 0)
	{
		int Err = WSAGetLastError();
		LOG(L"Network_WanClient", CLog::LEVEL_ERROR,
			L"Fail WSAStartup() ErrorCode : %d", Err);
		return false;
	}
	LOG(L"Network_WanClient", CLog::LEVEL_SYSTEM, L"WSAStartup()");

	// 입출력 완료 포트 생성
	hCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, numberOfConcurrentThreads);
	if (hCP == NULL)
	{
		int Err = GetLastError();
		LOG(L"Network_WanClient", CLog::LEVEL_ERROR, L"Fail CreateIoCompletionPort() ErrorCode : %d", Err);
		return 1;
	}

	// IOCP 워커스레드 생성
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

	// 타이머 스레드 생성
	hTimer_Threads = (HANDLE)_beginthreadex(nullptr, 0, TimerThreadWrapping, this, 0, nullptr);
	if (hTimer_Threads == 0)
	{
		int Err = errno;
		LOG(L"Network_WanClient", CLog::LEVEL_ERROR, L"Fail _beginthreadex() ErrorCode : %d", Err);
		return false;
	}

	// Client 소켓 생성
	_Session.socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_Session.socket == INVALID_SOCKET)
	{
		int Err = WSAGetLastError();
		LOG(L"Network_WanClient", CLog::LEVEL_ERROR,
			L"Fail socket() ErrorCode : %d", Err);
		return false;
	}
	LOG(L"Network_WanClient", CLog::LEVEL_SYSTEM, L"socket()");

	// 네이글 알고리즘 on/off
	int option = Nagle;
	setsockopt(_Session.socket,  //해당 소켓
		IPPROTO_TCP,			//소켓의 레벨
		TCP_NODELAY,			//설정 옵션
		(const char*)&option,	// 옵션 포인터
		sizeof(option));		//옵션 크기

	// 비동기 I/O를 위한 TCP SendBufSize = 0
	if (ZeroCopySend)
	{
		int optval = 0;	// send 버퍼 사이즈
		setsockopt(_Session.socket, SOL_SOCKET, SO_SNDBUF, (char*)&optval, sizeof(optval));
	}

	// 소켓과 입출력 완료 포트 연결
	CreateIoCompletionPort((HANDLE)_Session.socket, hCP, (ULONG_PTR)&_Session, 0);

	// IP 이진 형태 변환
	ULONG Ip;
	InetPton(AF_INET, IP, &Ip);

	// Client 소켓 주소 초기화
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

	// 서버와 연결 성공
	OnConnectServer();

	// 비동기 입출력 시작
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

	// Monitor용
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

	// Monitor용
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

// 네트워크의 에러가 발생할 시 로깅을 위한 함수입니다.
void CWanClient::NetworkError(int Err, const WCHAR* FunctionName)
{
	//Err == WSAEINVAL						// 10022 : WSASend에서 dwBufferCount 인수에 '0'이 들어갔을 떄 나타나는 것을 확인

	if (Err != WSAECONNRESET &&				// 10054 : 클라 측에서 끊었을 경우
		Err != WSAECONNABORTED &&			// 10053 : 소프트웨어(TCP)로 연결이 중단된 경우
		Err != ERROR_NETNAME_DELETED &&		// 64	 : RST, 클라에서 종료 했을 경우
		Err != WSAENOTSOCK &&				// 10038 : 소켓이 아닌 항목에서 작업을 시도했습니다.
		Err != ERROR_OPERATION_ABORTED &&	// 995	 : 스레드 종료 또는 애플리케이션 요청으로 인해 I/O 작업이 중단되었습니다. (CancelIoEx)
		Err != ERROR_SUCCESS)				// 0	 : 작업이 성공적으로 완료되었습니다. (FIN을 받았을 경우)
	{
		LOG(L"Network_WanClient", CLog::LEVEL_ERROR, L"Fail %s ErrorCode : %d", FunctionName, Err);
	}
	return;
}

// 네트워크 메시지의 헤더 확인해서 컨텐츠 Paload 추출 및 전달합니다.
void CWanClient::NetworkMessageProc()
{
	while (1)
	{
		// NetworkHeader 보다 크게 있는지 확인
		if (sizeof(NetworkHeader) >= _Session.RecvQ.GetUseSize())
			break;

		// Network Header peek
		NetworkHeader NetworkHeader;
		int retpeek = _Session.RecvQ.peek(&NetworkHeader, sizeof(NetworkHeader));

		// PayLoad가 들어왔는지 확인
		if (_Session.RecvQ.GetUseSize() - sizeof(NetworkHeader) >= NetworkHeader.Len)
		{
			// Network 헤더 크기만큼 이동
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
				// decode 실패
				LOG(L"Network_WanServer", CLog::LEVEL_DEBUG, L"Failed decode()");
				Packet->Free();
				Disconnect();
				break;
			}

			CPacket packet(Packet);
			OnRecv(packet);

			// Monitor 용
			InterlockedIncrement(&Pnd.RecvTPS);
		}
		else break;
	}

	return;
}

// WSARecv 보내기
void CWanClient::RecvPost()
{
	// 비동기 입출력 시작
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
			// WSARecv를 실패 했을 경우 IOCount 감소
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

//  WSASend 보내기
void CWanClient::SendPost()
{
	/*
	Send의 1회 제한
	1회로 제한 하지 않으면 WSASend 완료통지가 와서 링버퍼의 front가 처리 되기 전에 WSARecv 완료통지가
	먼저 와서 메시지 처리 후 WSARecv완료통지 쪽에서 다시 한번 WSASend를 하게 되면 이전 WSASend의 front가
	처리 되기 전에 Send를 하게 되어 메시지가 중첩된게 보내질 수 있다.
	*/
	SerialRingBuffer* SendQ = &_Session.SendQ;

	/*
		만약 지역변수로 UseSize를 사용하지 않으면, SendMSG로 인해 SendPost가 호출 되면 Session에 lock이 걸려있어서 상관 없지만
		WorkerThread에서 Send완료통지에서 SendPost를 호출 하는 경우 Session에 lock이 걸려있지 않기 때문에 모든 라인에서 GetUs
		esize()의 크기가 달라질 수 있다.
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

	// SerialRingBuffer의 point를 SendQ에 넣습니다.
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

// Session 삭제
void CWanClient::SessionRelease()
{
	DWORD CompareIOCount = IOCOUNT(0, 0);
	DWORD ExchangeIOCount = IOCOUNT(1, 0);
	DWORD retIOCount = InterlockedCompareExchange(&_Session.IOCount, ExchangeIOCount, CompareIOCount);
	if (CompareIOCount != retIOCount)
		return;

	closesocket(_Session.socket);
	_Session.socket = INVALID_SOCKET;


	// SendQ의 SerialRingBuffer RefCount Sub 및 SendQ에서 지우기
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

// 출력창에 출력합니다.
void CWanClient::OutputDebugStringF(LPCTSTR format, ...)
{
	WCHAR buffer[1000] = { 0 };

	va_list arg; // 가변인수 벡터

	va_start(arg, format);
	vswprintf_s(buffer, 1000, format, arg);
	va_end(arg);

	OutputDebugString(buffer);
	return;
}

// 함수 내부에서 일부로 크러쉬를 냅니다. 발생 하면 코드의 결함입니다.
void CWanClient::Crash()
{
	// 이 코드는 null 포인터를 역참조하여 크래시를 발생시킵니다.
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

		// 비동기 입출력(I/O) 대기
		retval = GetQueuedCompletionStatus(hCP, &cbTransferred,
			(PULONG_PTR)&pSession, (LPOVERLAPPED*)&Overlapped, INFINITE);

		// GQCS 오류로 인한 실패
		if (Overlapped == nullptr && retval == false)
		{
			int Err = GetLastError();
			LOG(L"Network_WanClient", CLog::LEVEL_SYSTEM, L"Failed GQCS ErrorCode : %d", Err);
			PostQueuedCompletionStatus(hCP, -1, 0, 0);
			break;
		}

		// PQCS 구분 (cbTransferred는 항상 -1으로 두고 Key(pSession), Ovelapped를 바꾼다 ex) -1, 0, 0)
		if (retval == true && cbTransferred == -1)
		{
			// WorkerThread 종료
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

			// 예상치 못한 PQCS가 왔을 경우
			LOG(L"Network_WanClient", CLog::LEVEL_ERROR,
				L"Unexpected PQCS [retGQCS = %d] [cbTransferred = %ld] [Overlapped = %lld]"
				, retval, cbTransferred, Overlapped);
			Crash();
		}

		// 세션 종료 : 클라에서 RST 보낸 경우 (실패한 I/O에 대한 완료통지를 처리합니다.)
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

			// 실패한 완료통지에 대한 IOCount 감소시킵니다.
			if (0 == InterlockedDecrement(&_Session.IOCount))
				SessionRelease();
			continue;
		}

		// WSARecv 완료   
		if (Overlapped == &_Session.RecvOverlapped)
		{
			// RecvQ rear 이동
			_Session.RecvQ.MoveRear(cbTransferred);

			// 메시지 처리
			NetworkMessageProc();

			// Recv 다시 걸어 둡니다.
			RecvPost();
			continue;
		}

		// WSASend 완료
		if (Overlapped == &_Session.SendOverlapped)
		{
			// SendQ의 SerialRingBuffer RefCount Sub 및 SendQ에서 지우기
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

			// SendFlag 해제
			InterlockedExchange(&_Session.SendFlag, 0);
			if (0 != _Session.SendQ.GetUseSize())
				SendPost();

			// 성공한 완료통지에 대한 IOCount 감소시킵니다.
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