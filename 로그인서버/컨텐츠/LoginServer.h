#ifndef __LOGINSERVER__
#define __LOGINSERVER__
#include <vector>

#include "네트워크/CWanServer.h"
#include "공용 라이브러리/CTlsPool.h"

#include "DB.h"
#include "LoginProtocol.h"

class LoginServer : public CWanServer
{
public:
	CTlsPool<mysql> mysqlPool;

	ServerSetting serverSet;
	BYTE LogLevel;

public:		
	LoginServer();
	~LoginServer();

private:	
	// 화이트 IP와 Port 접속 여부를 판단합니다. (Accpet 후)
	bool OnAcceptRequest(WCHAR* IP, WORD Port) override;
	// Accept 후 Session이 생성된 후 컨텐츠에 Client의 Session 생성을 알립니다.
	void OnSessionConnected(DWORD64 SessionID) override;
	// Session이 삭제 된 후 호출합니다.
	void OnSessionRelease(DWORD64 SessionID) override;
	// Message 수신 완료 후 Contents Packet 처리를 합니다.
	void OnRecv(DWORD64 SessionID, CPacket& packet) override;



private:
	// packet
	void Packet_Login(DWORD64 SessionID, CPacket& packet);
	void Packet_SelectServer(DWORD64 SessionID, CPacket& packet);
};

#endif