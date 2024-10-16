#ifndef __LOGINSERVER__
#define __LOGINSERVER__
#include <vector>

#include "��Ʈ��ũ/CWanServer.h"
#include "���� ���̺귯��/CTlsPool.h"

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
	// ȭ��Ʈ IP�� Port ���� ���θ� �Ǵ��մϴ�. (Accpet ��)
	bool OnAcceptRequest(WCHAR* IP, WORD Port) override;
	// Accept �� Session�� ������ �� �������� Client�� Session ������ �˸��ϴ�.
	void OnSessionConnected(DWORD64 SessionID) override;
	// Session�� ���� �� �� ȣ���մϴ�.
	void OnSessionRelease(DWORD64 SessionID) override;
	// Message ���� �Ϸ� �� Contents Packet ó���� �մϴ�.
	void OnRecv(DWORD64 SessionID, CPacket& packet) override;



private:
	// packet
	void Packet_Login(DWORD64 SessionID, CPacket& packet);
	void Packet_SelectServer(DWORD64 SessionID, CPacket& packet);
};

#endif