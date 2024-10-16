#ifndef __LOBBYSERVER_LOBBYSERVER_H__
#define __LOBBYSERVER_LOBBYSERVER_H__

// stl
#include <vector>
#include <set>
#include <map>
#include <queue>
#include <string>
#include <memory>
#include <optional>
using std::vector;
using std::set;
using std::map;
using std::queue;
using std::string;
using std::optional;
using std::to_string;


// ���̺귯��
#include "��Ʈ��ũ/CWanServerRoom2.h"
#include "���� ���̺귯��/CTlsPool.h"

#include "Aliases.h"
#include "LoginStatus.h"
class st_User;
struct st_Friend;
struct st_ServerStatus;
enum class en_ROOM_NUMBER : unsigned int;
enum class en_REDIS_TASK_TYPE;

class CLobbyServer : public CWanServerRoom2
{
public:
	CTlsPool<st_User>			UserPool;

	vector<std::pair<en_REDIS_TASK_TYPE, string >> RedisTaskKeys;

	// Server Setting
	string			ServerName;
	ServerSetting	ServerSet;
	BYTE			LogLevel;

	// �ϵ���� ����͸� ������
	CCpuUsage CpuUsage;


public:		CLobbyServer();
public:		~CLobbyServer();

			// ȭ��Ʈ IP�� Port ���� ���θ� �Ǵ��մϴ�. (Accpet ��)
private:	bool OnAcceptRequest(WCHAR* IP, WORD Port) override;

			
public:	
	st_User* allocUser();

	void freeUser(st_User* user);


public:		bool isDuplicationLogin(UID uid);

			// Redis�� �α��� ���� ���
public:		bool Redis_SetLoginStatus(const UID& uid, const Sid& SessionID);

public:		bool Redis_SetLoginStatus(const string& nickname, const Sid& SessionID);

			// Redis�� �α��� ���� ����
public:		void Redis_RemoveLoginStatus(const UID& uid, const string& Nickname);

public:		void Redis_RemoveLoginStatus(const string& Nickname);


			// Redis�� �α��� ���� uid �˻� (0 ��ȯ �� offline ����)
//public:		UID Redis_GetUID(const string& nickname);

			// Redis�� uid ������ �α��� ������ Ȯ��
public:		std::optional<st_LoginStatus> Redis_GetLoginStatus(const UID& uid);

public:		std::optional<st_LoginStatus> Redis_GetLoginStatus(const string& nickname);

			// Redis�� ģ�� ����� �α��� ������ Ȯ��
public:		void Redis_GetLoginStatus(vector<st_Friend>& Friends);

			// Redis�� ��ū ���
public:		DWORD64 Redis_SetToken(const UID& uid);

			// Redis���� ���� ���¸� �����´�
public:		vector<st_ServerStatus> Redis_GetServerStatus();

public:		bool Redis_Task_SetDuplicationLogin(const UID& uid);

public:		bool Redis_Task_SetWhisper(const string& SendNickname, const string& RecvNickname, const string& Chatting);

public:		bool Redis_Task_SetAddFriend(const string& SendNickname, const string& RecvNickname);

public:		bool Redis_Task_SetAcceptAddFriend(const string& SendNickname, const string& RecvNickname);

			// key�� �ش��ϴ� Task list�� �����´�.
private:	vector<string> Redis_Task_GetAndDelete(const string& key);

			// redis�� �ִ� Task ó��
private:	void Redis_Task_Execute();

			// redis�� ���� ���� ����
private:	void Redis_UpdateServerStatus();
};

// User*�� ���� Ŀ���� ������
//void CustomDeleter(st_User* ptr);



#endif // __LOBBYSERVER_LOBBYSERVER_H__
