#ifndef __LOGINSERVER_LOGINPROTOCOL_H__
#define __LOGINSERVER_LOGINPROTOCOL_H__
#include <Windows.h>

enum en_PACEKT_LOGIN_TYPE : short
{
	/*
	* 로그인 서버 로그인 요청
	*
	*	WORD		type
	*	char		id[20]
	*	char		password[20]
	*
	*/
	en_PACKET_CS_LOGIN_REQ_LOGIN = 0,
	/*
	* 
	*	WORD		type
	*	BYTE		status			// 하단에  정의
	*	DWORD64		uid
	*	DWORD64		serverCount
	*	// 아래 데이터는 serverCount 만큼 보냅니다.
	*	{
	*		char	ip[16]
	*		WORD	port
	*		char	serverName[20]
	*		BYTE	serverStatus
	*	}
	*/
	en_PACKET_SC_LOGIN_RES_LOGIN,



	/*
	* 
	*	WORD		type
	*	DWORD64		uid
	*  
	*/
	en_PACKET_CS_LOGIN_REQ_SELECT_SERVER,
	/*
	* 
	*	WORD		type
	*	char		token[64]
	* 
	*/
	en_PACKET_SC_LOGIN_RES_SELECT_SERVER
};


enum class en_STATUS_LOGIN : BYTE
{
	SUECCESS,
	NO_EXIST_ID,
	NO_EXIST_PASSWORD
};

#endif
