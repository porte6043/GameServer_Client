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


// 라이브러리
#include "네트워크/CWanServerRoom2.h"
#include "공용 라이브러리/CTlsPool.h"

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

	// 하드웨어 모니터링 데이터
	CCpuUsage CpuUsage;


public:		CLobbyServer();
public:		~CLobbyServer();

			// 화이트 IP와 Port 접속 여부를 판단합니다. (Accpet 후)
private:	bool OnAcceptRequest(WCHAR* IP, WORD Port) override;

			
public:	
	st_User* allocUser();

	void freeUser(st_User* user);


public:		bool isDuplicationLogin(UID uid);

			// Redis에 로그인 상태 등록
public:		bool Redis_SetLoginStatus(const UID& uid, const Sid& SessionID);

public:		bool Redis_SetLoginStatus(const string& nickname, const Sid& SessionID);

			// Redis에 로그인 상태 삭제
public:		void Redis_RemoveLoginStatus(const UID& uid, const string& Nickname);

public:		void Redis_RemoveLoginStatus(const string& Nickname);


			// Redis에 로그인 유저 uid 검색 (0 반환 시 offline 유저)
//public:		UID Redis_GetUID(const string& nickname);

			// Redis에 uid 유저가 로그인 중인지 확인
public:		std::optional<st_LoginStatus> Redis_GetLoginStatus(const UID& uid);

public:		std::optional<st_LoginStatus> Redis_GetLoginStatus(const string& nickname);

			// Redis에 친구 목록이 로그인 중인지 확인
public:		void Redis_GetLoginStatus(vector<st_Friend>& Friends);

			// Redis에 토큰 등록
public:		DWORD64 Redis_SetToken(const UID& uid);

			// Redis에서 서버 상태를 가져온다
public:		vector<st_ServerStatus> Redis_GetServerStatus();

public:		bool Redis_Task_SetDuplicationLogin(const UID& uid);

public:		bool Redis_Task_SetWhisper(const string& SendNickname, const string& RecvNickname, const string& Chatting);

public:		bool Redis_Task_SetAddFriend(const string& SendNickname, const string& RecvNickname);

public:		bool Redis_Task_SetAcceptAddFriend(const string& SendNickname, const string& RecvNickname);

			// key에 해당하는 Task list를 가져온다.
private:	vector<string> Redis_Task_GetAndDelete(const string& key);

			// redis에 있는 Task 처리
private:	void Redis_Task_Execute();

			// redis에 서버 상태 갱신
private:	void Redis_UpdateServerStatus();
};

// User*을 위한 커스텀 삭제자
//void CustomDeleter(st_User* ptr);



#endif // __LOBBYSERVER_LOBBYSERVER_H__
