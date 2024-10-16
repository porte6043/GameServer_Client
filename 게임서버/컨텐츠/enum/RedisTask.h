#ifndef  __LOBBYSERVER_REDISTASK_H__
#define __LOBBYSERVER_REDISTASK_H__
// stl
#include <string>
using std::string;

// 전역함수
#include "StringUtilities.h"

// 정의 및 프로토콜
#include "Aliases.h"

struct st_RedisTask_Whisper
{
	Sid		SessionID;
	string	SendNickname;
	string	Chatting;

	st_RedisTask_Whisper() = delete;
	st_RedisTask_Whisper(const string& task)
	{
		auto TaskElements = SplitString(task, ':');
		SessionID = std::stoull(TaskElements[0]);
		SendNickname = TaskElements[1];
		Chatting = TaskElements[2];
	}
};

struct st_RedisTask_AddFriend
{
	Sid		SessionID;
	string	SendNickname;

	st_RedisTask_AddFriend() = delete;
	st_RedisTask_AddFriend(const string& task)
	{
		auto TaskElements = SplitString(task, ':');
		SessionID = std::stoull(TaskElements[0]);
		SendNickname = TaskElements[1];
	}
};

struct st_RedisTask_AcceptAddFriend
{
	Sid		SessionID;
	string	SendNickname;

	st_RedisTask_AcceptAddFriend() = delete;
	st_RedisTask_AcceptAddFriend(const string& task)
	{
		auto TaskElements = SplitString(task, ':');
		SessionID = std::stoull(TaskElements[0]);
		SendNickname = TaskElements[1];
	}
};

struct st_RedisTask_DuplicationLogin
{
	Sid		SessionID;

	st_RedisTask_DuplicationLogin() = delete;
	st_RedisTask_DuplicationLogin(const string& task)
	{
		auto TaskElements = SplitString(task, ':');
		SessionID = std::stoull(TaskElements[0]);
	}
};


#endif //  __LOBBYSERVER_REDISTASK_H__