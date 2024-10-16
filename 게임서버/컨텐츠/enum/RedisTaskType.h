#ifndef __LOBBYSERVER_REDISTASKTYPE_H__
#define __LOBBYSERVER_REDISTASKTYPE_H__

enum class en_REDIS_TASK_TYPE
{
	WHISPER = 1,			// { SessionID :SendNickname :Chatting }

	ADD_FRIEND,				// { SessionID :SendNickname }

	ACCEPT_ADD_FRIEND,		// { SessionID :SendNickname }

	DUPLICATION_LOGIN		// { SessionID }
};

#endif
