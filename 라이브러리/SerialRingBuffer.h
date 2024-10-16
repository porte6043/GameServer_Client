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
	// 전체 버퍼 크기 얻기
	int GetBufferSize();

	// 현재 버퍼에 사용중인 용량 얻기
	int GetUseSize();

	// 현재 버퍼에 사용 가능량 얻기
	int GetFreeSize();

	// 반환값 : 실제로 enqueue 된 사이즈
	int enqueue(const void* pData, int Size);

	// 반환값 : 실제로 dequeue 된 사이즈
	int dequeue(void* pDest, int Size);
	int dequeue(std::string& pDest, int Size);

	int peek(void* chpdest, int chpdestsize);

	// 버퍼의 모든 데이터 삭제
	void clear();

	// buffer의 header 포인터 얻음
	char* LanHead();

	char* WanHead();

	// buffer의 front 포인터 얻음
	char* front();

	// buffer의 rear 포인터 얻음
	char* rear();

	// buffer의 포인터 얻음
	char* buffer();

	// size만큼 front 이동
	void MoveFront(int size);

	// size만큼 rear 이동
	void MoveRear(int size);

	// Rear 포인터로 직접적으로 이동할 수 있는 사이즈
	int DirectEnqueueSize();

	// Front 포인터로 직접적으로 이동할 수 있는 사이즈
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
	CPacket();											// 기본 생성자
	CPacket(SerialRingBuffer* packet);					// 초기화 생성자
	CPacket(CPacket& other);							// Reference 생성자
	CPacket(const CPacket& other);						// 복사 생성자 (std에서 사용)
	CPacket& operator=(const CPacket& other);			// 복사 대입 연산자
	CPacket& operator=(const SerialRingBuffer* packet);	// 복사 대입 연산자
	~CPacket();											// 소멸자
	//============================================================================================
	// 생성자 예시
	// CPacket(SerialRingBuffer* packet);					-> CPacket t(CPacket::Alloc());
	// CPacket(CPacket& other);								-> CPacket t = t1; OR 기본 함수의 인자
	// CPacket& operator=(const CPacket& other);			-> t = t1;
	// CPacket& operator=(const SerialRingBuffer* packet)	-> CPacket t; \ t = CPacket::Alloc();
	//============================================================================================

	
	
public:
	static SerialRingBuffer* Alloc();
	int GetUseSize();
	void clear();

	// 데이터 삽입
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

	 

	// 데이터 꺼내기
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