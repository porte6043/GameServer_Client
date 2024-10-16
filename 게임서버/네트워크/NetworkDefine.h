#ifndef __NETWORK_NETWORKDEFINE_H__
#define __NETWORK_NETWORKDEFINE_H__

#define SESSIONID(SessionID, SessionIndex) ( ((DWORD64)SessionID << 32) | (DWORD64)SessionIndex )
#define SESSIONID_ID_HIGH(SessionID) (DWORD)(SessionID >> 32)
#define SESSIONID_INDEX_LOW(SessionID) (DWORD)(SessionID & 0x0000'0000'ffff'ffff)

#define IOCOUNT(Flag, IOCount) ( ((DWORD)Flag << 16) |  (DWORD)IOCount )
#define IOCOUNT_FLAG_HIGH(IOCount) (DWORD)(IOCount >> 16)
#define IOCOUNT_IOCOUNT_LOW(IOCount) (DWORD)(IOCount & 0x0000'ffff)

#define INVALID_SESSIONID 0
#define INVALID_ROOMNO -1
#define PQCS_TRANSFERRED -1

enum class en_JOB_ROOM
{
	ENTER,					// Room�� ���� ��
	ROOM_DELETE,			// Room�� ������ ��
	ROOM_MESSAGE,			// Room�� �����͸� ���� ��
	ROOM_MESSAGE_SESSION	// Room�� �����ϴ� Session���� �����͸� ���� ��
};


#endif