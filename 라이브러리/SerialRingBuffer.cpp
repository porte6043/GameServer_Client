#include "SerialRingBuffer.h"
#include <unordered_map>
#include <list>

#ifdef __CTLSPOOL__
CTlsPool<SerialRingBuffer> g_Pool(5000, 20);
BYTE PacketKey = 0x00;
#endif

 
// Read Point = Front
// Write Point = Rear

SerialRingBuffer::SerialRingBuffer() : HeaderSize(5) , RefCount(0), HeaderFlag(0), EncodeFlag(0)
{	
	int bufferSize = en_BUFFER_DEFAULT_SIZE;
	Header = new char[bufferSize];
	memset(Header, 0, bufferSize);
	Buffer = Header + HeaderSize;
	Front = Header + HeaderSize;
	Rear = Header + HeaderSize;
	BufferTotalSize = bufferSize - HeaderSize - 1; // ringbuffer�� �ϳ��� ����ִ� ���� + ��� �κ� ����ִ� ����
	InitializeCriticalSection(&CS_SerialRingBuffer);
}
SerialRingBuffer::SerialRingBuffer(int BufferSize) : HeaderSize(5), RefCount(0), HeaderFlag(0), EncodeFlag(0)
{
	int bufferSize = BufferSize;
	Header = new char[bufferSize];
	memset(Header, 0, bufferSize);
	Buffer = Header + HeaderSize;
	Front = Header + HeaderSize;
	Rear = Header + HeaderSize;
	BufferTotalSize = bufferSize - HeaderSize - 1; // ringbuffer�� �ϳ��� ����ִ� ���� + ��� �κ� ����ִ� ����
	InitializeCriticalSection(&CS_SerialRingBuffer);
}

SerialRingBuffer::~SerialRingBuffer()
{
	delete[] Header;
}

int SerialRingBuffer::GetBufferSize()
{
	return BufferTotalSize + HeaderSize + 1;
}

int SerialRingBuffer::GetUseSize()
{
	int UseSize;
	char* pFront = Front;
	char* pRear = Rear;

	if (pFront <= pRear)	// Rear�� �����ʿ� ���� ��
		UseSize = (int)(pRear - pFront);
	else					// Rear�� ���ʿ� ���� ��
		UseSize = BufferTotalSize + 1 - (int)(pFront - pRear);


	return UseSize;
}

int SerialRingBuffer::GetFreeSize()
{
	int FreeSize;
	char* pFront = Front;
	char* pRear = Rear;

	if (pFront <= pRear)	// Rear�� �����ʿ� ���� ��
	{
		FreeSize = BufferTotalSize - (int)(pRear - pFront);
	}
	else					// Rear�� ���ʿ� ���� ��
	{
		FreeSize = (int)(pFront - pRear) - 1;
	}


	return FreeSize;
}

int SerialRingBuffer::enqueue(const void* pData, int Size)
{
	// enqueue size�� 0�̸� return
	if (Size <= 0) return 0;

	int EnqueueSize; // RingBuffer�� ���� ���� �� ������

	int FreeSize = GetFreeSize(); // ��� ������ ���� ������
	if (FreeSize >= Size)
		EnqueueSize = Size;
	else
		EnqueueSize = FreeSize;

	// ���۰� FULL�̸� return
	if (EnqueueSize <= 0)
		return 0;


	char* pRear = Rear;
	int DirectSize = DirectEnqueueSize();
	if (DirectSize >= EnqueueSize)	// �и� �ȵǰ� �ѹ��� memcpy
	{
		memcpy(pRear, pData, EnqueueSize);
		if (pRear + EnqueueSize == Buffer + BufferTotalSize + 1)
			Rear = Buffer;
		else
			Rear += EnqueueSize;
	}
	else							// �и� �Ǿ� �ι��� memcpy
	{
		memcpy(pRear, pData, DirectSize);
		memcpy(Buffer, (char*)pData + DirectSize, EnqueueSize - DirectSize);
		Rear = Buffer + EnqueueSize - DirectSize;
	}

	return EnqueueSize;
}

int SerialRingBuffer::dequeue(void* pDest, int Size)
{
	// enqueue size�� 0�̸� return
	if (Size <= 0) return 0;

	int DequeueSize; // chpdata�� ���� �� ������
	int UseSize = GetUseSize(); // dequeue ������ ������

	if (UseSize == 0 || Size == 0)
		return 0;

	// ���� chpdata ���� �� ������
	if (UseSize >= Size)
	{
		DequeueSize = Size;
	}
	else
	{
		DequeueSize = UseSize;
	}

	char* pFront = Front;
	int DirectSize = DirectDequeueSize();
	if (DirectSize >= DequeueSize)
	{
		memcpy(pDest, pFront, DequeueSize);
		if (pFront + DequeueSize == Buffer + BufferTotalSize + 1)
			Front = Buffer;
		else
			Front += DequeueSize;
	}
	else
	{
		memcpy(pDest, pFront, DirectSize);
		memcpy((char*)pDest + DirectSize, Buffer, DequeueSize - DirectSize);
		Front = Buffer + DequeueSize - DirectSize;
	}


	return DequeueSize;
}
int SerialRingBuffer::dequeue(std::string& pDest, int Size)
{
	// enqueue size�� 0�̸� return
	if (Size <= 0) return 0;

	int DequeueSize; // chpdata�� ���� �� ������
	int UseSize = GetUseSize(); // dequeue ������ ������

	if (UseSize == 0 || Size == 0)
		return 0;

	// ���� chpdata ���� �� ������
	if (UseSize >= Size)
	{
		DequeueSize = Size;
	}
	else
	{
		DequeueSize = UseSize;
	}

	char* pFront = Front;
	int DirectSize = DirectDequeueSize();
	if (DirectSize >= DequeueSize)
	{
		std::string str(pFront, DequeueSize);
		str.erase(str.find_first_of('\0'));
		pDest = std::move(str);
		if (pFront + DequeueSize == Buffer + BufferTotalSize + 1)
			Front = Buffer;
		else
			Front += DequeueSize;
	}
	else
	{
		std::string str(pFront, DirectSize);
		str.append(Buffer, DequeueSize - DirectSize);
		str.erase(str.find_first_of('\0'));
		pDest = std::move(str);
		Front = Buffer + DequeueSize - DirectSize;
	}


	return DequeueSize;
}

int SerialRingBuffer::peek(void* chpdest, int chpdestsize)
{
	int PeekSize;
	char* PeekPos;
	int UseSize = GetUseSize();
	if (UseSize == 0 || chpdestsize == 0) return 0;

	if (UseSize >= chpdestsize)
	{
		PeekSize = chpdestsize;
	}
	else
	{
		PeekSize = UseSize;
	}

	char* pFront = Front;
	int DirectSize = DirectDequeueSize();
	if (DirectSize >= PeekSize)
	{
		memcpy(chpdest, pFront, PeekSize);
	}
	else
	{
		memcpy(chpdest, pFront, DirectSize);
		memcpy((char*)chpdest + DirectSize, Buffer, PeekSize - DirectSize);
	}


	return PeekSize;
}

void SerialRingBuffer::clear()
{
	Front = Buffer;
	Rear = Buffer;
	HeaderFlag = 0;
	EncodeFlag = 0;
	//ZeroMemory(Header, 6400);

	return;
}

char* SerialRingBuffer::WanHead()
{
	return Header;
}

char* SerialRingBuffer::LanHead()
{
	return Header + HeaderSize - 2;
}

char* SerialRingBuffer::front()
{
	return Front;
}

char* SerialRingBuffer::rear()
{
	return Rear;
}

char* SerialRingBuffer::buffer()
{
	return Buffer;
}

void SerialRingBuffer::MoveFront(int size)
{
	if (size == 0)
		return;

	char* pFront = Front;
	int DirectSize = DirectDequeueSize();
	if (DirectSize >= size)
	{
		if (pFront + size == Buffer + BufferTotalSize + 1)
		{
			Front = Buffer;
		}
		else
		{
			Front += size;
		}
	}
	else
	{
		Front = Buffer + size - DirectSize;
	}

	return;
}

void SerialRingBuffer::MoveRear(int size)
{
	if (size == 0)
		return;

	char* pRear = Rear;
	int DirectSize = DirectEnqueueSize();
	if (DirectSize >= size)
	{
		if (pRear + size == Buffer + BufferTotalSize + 1)
			Rear = Buffer;
		else
			Rear += size;
	}
	else
	{
		Rear = Buffer + size - DirectSize;
	}

	return;
}

int SerialRingBuffer::DirectEnqueueSize()
{
	char* pFront = Front;
	char* pRear = Rear;
	int DirectSize;

	if (pFront <= pRear)
	{
		DirectSize = (int)(Buffer + BufferTotalSize + 1) - (int)pRear;
		//DirectSize = reinterpret_cast<int>(Buffer + BufferTotalSize + 1) - reinterpret_cast<int>(pRear);

		if (pFront == Buffer)
			--DirectSize;
	}
	else
	{
		DirectSize = pFront - pRear - 1;
	}

	return DirectSize;
}

int SerialRingBuffer::DirectDequeueSize()
{
	char* pFront = Front;
	char* pRear = Rear;
	int DirectSize;

	if (pFront <= pRear)
		DirectSize = (int)(pRear - pFront);
	else
		DirectSize = (int)(Buffer + BufferTotalSize + 1) - (int)pFront;

	return DirectSize;
}

void SerialRingBuffer::encode()
{
	if (EncodeFlag == 0)
	{
		EnterCriticalSection(&CS_SerialRingBuffer);
		if (EncodeFlag == 0)
		{
			char RandKey = *(Header + 3);
			char* CheckSum = Header + 4;
			char P = 0;
			char E = 0;
			char D;
			int UseSize = GetUseSize();
			for (int iCnt = 0; iCnt < UseSize + 1; ++iCnt)
			{
				D = *(CheckSum + iCnt);
				P = D ^ (P + RandKey + iCnt + 1);
				E = P ^ (E + PacketKey + iCnt + 1);

				*(CheckSum + iCnt) = E;
			}
			EncodeFlag = 1;
		}
		LeaveCriticalSection(&CS_SerialRingBuffer);
	}
}

bool SerialRingBuffer::decode()
{
	BYTE RandKey = *(Header + 3);

	char D;
	char P = 0;
	char E = 0;
	int Prev_E = 0;
	int Prev_P = 0;
	int UseSize = GetUseSize();
	for (int iCnt = 0; iCnt < UseSize + 1; ++iCnt)
	{
		E = *(Header + 4 + iCnt);
		P = E ^ (Prev_E + PacketKey + iCnt + 1);
		D = P ^ (Prev_P + RandKey + iCnt + 1);

		*(Header + 4 + iCnt) = D;
		Prev_E = E;
		Prev_P = P;
	}

	// üũ�� üũ
	char* byte = front();
	unsigned char CheckSum = 0;
	for (int iCnt = 0; iCnt < UseSize; ++iCnt)
	{
		CheckSum += *(byte + iCnt);
	}
	
	if (CheckSum != (unsigned char)*(Header + 4))
		return false;

	return true;
}

void SerialRingBuffer::SetWanHeader(const void* Src, int Size)
{
	if (Size != HeaderSize)
		Crash();

	if (HeaderFlag == 0)
	{
		EnterCriticalSection(&CS_SerialRingBuffer);
		if (HeaderFlag == 0)
		{
			memcpy(Header, Src, Size);
			HeaderFlag = 1;
		}
		LeaveCriticalSection(&CS_SerialRingBuffer);
	}

	return;
}

void SerialRingBuffer::SetLanHeader(const void* Src, int Size)
{
	if (Size != 2)
		Crash();

	if (HeaderFlag == 0)
	{
		EnterCriticalSection(&CS_SerialRingBuffer);
		if (HeaderFlag == 0)
		{
			memcpy(Header + HeaderSize - Size, Src, Size);
			HeaderFlag = 1;
		}
		LeaveCriticalSection(&CS_SerialRingBuffer);
	}

	return;
}


SerialRingBuffer* SerialRingBuffer::Alloc()
{
	SerialRingBuffer* pBuffer = g_Pool.Alloc();
	pBuffer->clear();
	return pBuffer;
}

void SerialRingBuffer::Free()
{
	g_Pool.Free(this);
	return;
}

void SerialRingBuffer::AddRef()
{
	InterlockedIncrement(&RefCount);
	return;
}

void SerialRingBuffer::SubRef()
{
	if (0 == InterlockedDecrement(&RefCount))
		Free();

	return;
}

void SerialRingBuffer::Crash()
{
	int* ptr = nullptr;
	*ptr = 10;  // �� �ڵ�� null �����͸� �������Ͽ� ũ���ø� �߻���ŵ�ϴ�.
}

int SerialRingBuffer::GetPoolTotalCount()
{
	return g_Pool.GetTotalCount();
}

int SerialRingBuffer::GetPoolUseCount()
{
	return g_Pool.GetUseCount();
}

void SerialRingBuffer::SetPacketKey(BYTE packetKey)
{
	PacketKey = packetKey;
	return;
}


//------------------------------------------------------------
CPacket::CPacket()									// �⺻ ������
{
	Packet = SerialRingBuffer::Alloc();
	Packet->AddRef();
}
CPacket::CPacket(CPacket& other)					// Reference ������ -> �⺻�Լ����ڷ� ����
{
	Packet = other.Packet;
	Packet->AddRef();
}
CPacket::CPacket(const CPacket& other)
{
	Packet = other.Packet;
	if (Packet != nullptr)
	{
		Packet->AddRef();
	}
}
CPacket::CPacket(SerialRingBuffer* packet)			// �ʱ�ȭ ������ : Test t1(t2); , Test t3 = t2;
{
	Packet = packet;
	Packet->AddRef();
}
CPacket& CPacket::operator=(const CPacket& other)	// ���� ���� ������
{
	Packet = other.Packet;
	Packet->AddRef();
	return *this;
}
CPacket& CPacket::operator=(const SerialRingBuffer* srPacket)	// ���� ���� ������
{
	Packet = const_cast<SerialRingBuffer*>(srPacket);
	Packet->AddRef();
	CPacket cPacket(Packet);
	return cPacket;
}
CPacket::~CPacket()									// �Ҹ���
{
	if(Packet != nullptr)
		Packet->SubRef();
}

SerialRingBuffer* CPacket::Alloc()
{
	return SerialRingBuffer::Alloc();
}

int CPacket::GetUseSize()
{
	return Packet->GetUseSize();
}
void CPacket::clear()
{
	Packet->clear();
	return;
}

// ������ ����
int CPacket::PutData(const void* pSrc, const int& Size)
{
	return Packet->enqueue(pSrc, Size);
}
int CPacket::PutData(const std::string& str, const int& Size)
{
	if (str.size() >= Size)
	{
		char null = '\0';
		Packet->enqueue(str.c_str(), Size - 1);
		Packet->enqueue(&null, 1);
	}
	else
	{
		char null = '\0';
		Packet->enqueue(str.c_str(), str.size());
		Packet->enqueue(&null, 1);
		Packet->MoveRear(Size - str.size() - 1);
	}

	return Size;
}
CPacket& CPacket::operator<<(const bool& boolValue)
{
	Packet->enqueue(&boolValue, sizeof(bool));
	return *this;
}
CPacket& CPacket::operator<<(const BYTE& chValue)
{
	Packet->enqueue(&chValue, sizeof(BYTE));
	return *this;
}
CPacket& CPacket::operator<<(const WORD& shValue)
{
	Packet->enqueue(&shValue, sizeof(WORD));
	return *this;
}
CPacket& CPacket::operator<<(const INT64& llValue)
{
	Packet->enqueue(&llValue, sizeof(INT64));
	return *this;
}
CPacket& CPacket::operator<<(const DWORD64& llValue)
{
	Packet->enqueue(&llValue, sizeof(DWORD64));
	return *this;
}
CPacket& CPacket::operator<<(const void* llValue)
{
	Packet->enqueue(&llValue, sizeof(void*));
	return *this;
}
CPacket& CPacket::operator<<(const int& lValue)
{
	Packet->enqueue(&lValue, sizeof(int));
	return *this;
}
CPacket& CPacket::operator<<(const WORD&& shValue)
{
	Packet->enqueue(&shValue, sizeof(WORD));
	return *this;
}

CPacket& CPacket::operator<<(const DWORD& lValue)
{
	Packet->enqueue(&lValue, sizeof(DWORD));
	return *this;
}

CPacket& CPacket::operator<<(const std::string& string)
{
	Packet->enqueue(string.c_str(), string.capacity());
	return *this;
}

// ������ ������
int CPacket::CPacket::GetData(void* pDest, int Size)
{
	return Packet->dequeue(pDest, Size);
}
int CPacket::GetData(std::string& pDest, int Size)
{
	return Packet->dequeue(pDest, Size);
}
CPacket& CPacket::operator>>(bool& boolValue)
{
	Packet->dequeue(&boolValue, sizeof(bool));
	return *this;
}
CPacket& CPacket::operator>>(BYTE& chValue)
{
	Packet->dequeue(&chValue, sizeof(BYTE));
	return *this;
}
CPacket& CPacket::operator>>(WORD& shValue)
{
	Packet->dequeue(&shValue, sizeof(WORD));
	return *this;
}
CPacket& CPacket::operator>>(INT64& llValue)
{
	Packet->dequeue(&llValue, sizeof(INT64));
	return *this;
}
CPacket& CPacket::operator>>(DWORD64& llValue)
{
	Packet->dequeue(&llValue, sizeof(DWORD64));
	return *this;
}
CPacket& CPacket::operator>>(void* llValue)
{
	Packet->dequeue(&llValue, sizeof(void*));
	return *this;
}
CPacket& CPacket::operator>>(int& lValue)
{
	Packet->dequeue(&lValue, sizeof(int));
	return *this;
}
CPacket& CPacket::operator>>(DWORD& lValue)
{
	Packet->dequeue(&lValue, sizeof(DWORD));
	return *this;
}