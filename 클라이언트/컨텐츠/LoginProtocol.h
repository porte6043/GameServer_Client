#ifndef __LOGINSERVER_LOGINPROTOCOL_H__
#define __LOGINSERVER_LOGINPROTOCOL_H__
#include <Windows.h>

enum en_PACEKT_LOGIN_TYPE : short
{
	/*
	* �α��� ���� �α��� ��û
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
	*	BYTE		status			// �ϴܿ�  ����
	*	DWORD64		uid
	*	DWORD64		serverCount
	*	// �Ʒ� �����ʹ� serverCount ��ŭ �����ϴ�.
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
