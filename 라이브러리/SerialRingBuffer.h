#ifndef __SERIAL_RINGBUFFER__
#define __SERIAL_RINGBUFFER__
#include <Windows.h>
#include "CTlsPool.h"


class CPacket;
class SerialRingBuffer;

class SerialRingBuffer
{
	friend class CWanServerRoom;
	friend class CWanServerRoom2;
	friend class CRoom;
	friend class CRoom2;
	friend class CWanServer;
	friend class CWanClient;
	friend class CLanServer;
	friend class CLanClient;
	friend class CPacket;

public:
	enum Buffer
	{
		en_BUFFER_DEFAULT_SIZE = 16384
	};


private:
	char* Header;
	char* Buffer;
	char* Front;
	char* Rear;
	int HeaderSize;
	int BufferTotalSize;
	long HeaderFlag;
	long EncodeFlag;
	CRITICAL_SECTION CS_SerialRingBuffer;
	long RefCount;

public:
	SerialRingBuffer();
	SerialRingBuffer(int BufferSize);

	~SerialRingBuffer();

private:
	// ��ü ���� ũ�� ���
	int GetBufferSize();

	// ���� ���ۿ� ������� �뷮 ���
	int GetUseSize();

	// ���� ���ۿ� ��� ���ɷ� ���
	int GetFreeSize();

	// ��ȯ�� : ������ enqueue �� ������
	int enqueue(const void* pData, int Size);

	// ��ȯ�� : ������ dequeue �� ������
	int dequeue(void* pDest, int Size);
	int dequeue(std::string& pDest, int Size);

	int peek(void* chpdest, int chpdestsize);

	// ������ ��� ������ ����
	void clear();

	// buffer�� header ������ ����
	char* LanHead();

	char* WanHead();

	// buffer�� front ������ ����
	char* front();

	// buffer�� rear ������ ����
	char* rear();

	// buffer�� ������ ����
	char* buffer();

	// size��ŭ front �̵�
	void MoveFront(int size);

	// size��ŭ rear �̵�
	void MoveRear(int size);

	// Rear �����ͷ� ���������� �̵��� �� �ִ� ������
	int DirectEnqueueSize();

	// Front �����ͷ� ���������� �̵��� �� �ִ� ������
	int DirectDequeueSize();

	void encode();

	bool decode();

	// Wan Network Header Set
	void SetWanHeader(const void* Src, int Size);

	// Lan Network Header Set
	void SetLanHeader(const void* Src, int Size);

	static SerialRingBuffer* Alloc();

	void Free();

	static void SetPacketKey(BYTE packetKey);

	void AddRef();

	void SubRef();

	void Crash();

public:		static int GetPoolTotalCount();
public:		static int GetPoolUseCount();

};



class CPacket 
{
	friend class CWanServerRoom;
	friend class CWanServerRoom2;
	friend class CRoom;
	friend class CRoom2;
	friend class CWanServer;
	friend class CWanClient;
	friend class CLanServer;
	friend class CLanClient;

private:
	SerialRingBuffer* Packet;

public:
	CPacket();											// �⺻ ������
	CPacket(SerialRingBuffer* packet);					// �ʱ�ȭ ������
	CPacket(CPacket& other);							// Reference ������
	CPacket(const CPacket& other);						// ���� ������ (std���� ���)
	CPacket& operator=(const CPacket& other);			// ���� ���� ������
	CPacket& operator=(const SerialRingBuffer* packet);	// ���� ���� ������
	~CPacket();											// �Ҹ���
	//============================================================================================
	// ������ ����
	// CPacket(SerialRingBuffer* packet);					-> CPacket t(CPacket::Alloc());
	// CPacket(CPacket& other);								-> CPacket t = t1; OR �⺻ �Լ��� ����
	// CPacket& operator=(const CPacket& other);			-> t = t1;
	// CPacket& operator=(const SerialRingBuffer* packet)	-> CPacket t; \ t = CPacket::Alloc();
	//============================================================================================

	
	
public:
	static SerialRingBuffer* Alloc();
	int GetUseSize();
	void clear();

	// ������ ����
	int PutData(const void* pSrc, const int& Size);
	int PutData(const std::string& str, const int& Size);
	CPacket& operator<<(const bool& boolValue);
	CPacket& operator<<(const BYTE& chValue);
	CPacket& operator<<(const WORD& shValue);
	CPacket& operator<<(const INT64& llValue);
	CPacket& operator<<(const DWORD64& llValue);
	CPacket& operator<<(const void* llValue);
	CPacket& operator<<(const int& lValue);
	CPacket& operator<<(const WORD&& shValue);
	CPacket& operator<<(const DWORD& lValue);
	CPacket& operator<<(const std::string& string);

	 

	// ������ ������
	int GetData(void* pDest, int Size);
	int GetData(std::string& pDest, int Size);
	CPacket& operator>>(bool& boolValue);
	CPacket& operator>>(BYTE& chValue);
	CPacket& operator>>(WORD& shValue);
	CPacket& operator>>(INT64& llValue);
	CPacket& operator>>(DWORD64& llValue);
	CPacket& operator>>(void* llValue);
	CPacket& operator>>(int& lValue);
	CPacket& operator>>(DWORD& lValue);

};
#endif