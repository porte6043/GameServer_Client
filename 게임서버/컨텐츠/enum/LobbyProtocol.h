#ifndef __LOBBYSERVER_LOBBYPROTOCOL_H__
#define __LOBBYSERVER_LOBBYPROTOCOL_H__



enum en_PACEKT_LOBBY_TYPE : short
{
	/*
	* �κ� ���� �α��� ��û
	*
	*	WORD	Type
	*	INT64	UID
	*	char	SessionKey[64]
	*
	*/
	en_PACKET_CS_LOBBY_REQ_LOGIN = 0,
	/*
	* �κ� ���� �α��� ����
	*
	*	WORD	Type
	*	DWORD64	CharaterCount			// ���� �� ĳ���� ��
	*	// �Ʒ� �����ʹ� ĳ���� �� ��ŭ �����ϴ�.
	*	{
	*		char	Nickname[20]		// ĳ���� �г��� (null ����)
	*		BYTE	CharacterSeq		// ĳ���� ����
	*	}
	*
	*/
	en_PACKET_SC_LOBBY_RES_LOGIN,

	/*
	* �κ� ���� ä�� �̵� ���� ��û
	*
	*	WORD	Type
	*	INT64	UID
	*	char	SessionKey[64]
	*	char	CharacterName[20]
	*
	*/
	en_PACKET_CS_LOBBY_REQ_LOGIN_OTHER_CHANNEL,
	/*
	* �κ� ���� ä�� �̵� ���� ����
	*
	*	WORD	Type
	*
	*/
	en_PACKET_SC_LOBBY_RES_LOGIN_OTHER_CHANNEL,


	/*
	* ���� ĳ���� ���� ��û
	*
	*	WORD	Type
	*	char	CharacterName[20]
	*
	*/
	en_PACKET_CS_LOBBY_REQ_SELECT_CHARACTER,
	/*
	* �κ� ���� ���� ĳ���� ���� ����
	*
	*	WORD	Type
	*	char	Nickname[20]			// ĳ���� �г��� (null ����)
	*	DWORD64	uid
	*	DWORD	RoomNo					// ĳ���� ��ġ
	*
	*	DWORD64	statsCount				// ĳ���� �ɷ�ġ
	*	// statsCount ��ŭ �����ϴ�.
	*	{
	*		BYTE	statsID
	*		DWORD	quantity
	*		char	statsName[20]
	*	}
	*
	*	DWORD64	moneyCount
	*	// moneyCount ��ŭ �����ϴ�.
	*	{
	*		BYTE	moneyID
	*		DWORD	quantity
	*		char	moneyName[20]
	*	}
	*
	*	DWORD	maxSlot					// ������ ���� ����
	*	DWORD64	ItemCount				// ������ ��
	*	// ItemCount ��ŭ �����ϴ�.
	*	{
	*		BYTE	itemType			// item type
	*		DWORD	itemID
	*		char	itemName[64]
	*		char	description[512]
	*		DWORD	maxQuantity
	*		DWORD	seq
	*		DWORD	quantity
	*	}
	*
	*	DWORD64	FriendCount				// ģ�� ��� ��
	*	// FriendCount ��ŭ �����ϴ�.
	*	{
	*		char	FriendNickname[20]	// ģ�� �г���
	*	}
	*
	*	DWORD64	AddFriendCount			// ģ�� ��� ��
	*	// AddFriendCount ��ŭ �����ϴ�.
	*	{
	*		char	FriendNickname[20]	// ģ�� �г���
	*	}
	*
	*/
	en_PACKET_SC_LOBBY_RES_SELECT_CHARACTER,

	/*
	* ĳ���� ����Ʈ ��û
	*
	*	WORD	type
	*
	*/
	en_PACKET_CS_LOBBY_REQ_CHARACTERLIST,
	/*
	* ĳ���� ����Ʈ ����
	*
	*	WORD	type
	*	DWORD64	CharaterCount			// ���� �� ĳ���� ��
	*	// �Ʒ� �����ʹ� ĳ���� �� ��ŭ �����ϴ�.
	*	{
	*		char	Nickname[20]		// ĳ���� �г��� (null ����)
	*		BYTE	CharacterSeq		// ĳ���� ����
	*	}
	*
	*/
	en_PACKET_SC_LOBBY_RES_CHARACTERLIST,


	/*
	* �κ� ���� ��� ������ ĳ���� �̸� Ȯ�� ��û
	*
	*	WORD	Type
	*	char	Nickname[20]
	*
	*/
	en_PACKET_CS_LOBBY_REQ_AVAILABLE_NICKNAME,
	/*
	* �κ� ���� ��� ������ ĳ���� �̸� Ȯ�� ����
	*
	*	WORD	Type
	*	BYTE	Status					// (true, false)
	*
	*/
	en_PACKET_SC_LOBBY_RES_AVAILABLE_NICKNAME,


	/*
	* �κ� ���� ĳ���� ���� ��û
	*
	*	WORD	Type
	*	UID		UID
	*	char	Nickname[20]			// ������ ĳ���� �̸�
	*	WORD	CharacterSeq			// ������ ĳ���� ����
	*
	*/
	en_PACKET_CS_LOBBY_REQ_CREATE_CHARACTER,
	/*
	* �κ� ���� ĳ���� ���� ����
	*
	*	WORD	Type
	*	BYTE	Status					// ���� ���� ����	 (�Ʒ� ���� �Ǿ� �ֽ��ϴ�.)
	*
	*/
	en_PACKET_SC_LOBBY_RES_CREATE_CHARACTER,

	// Lobby Room
	//-------------------------------------------------------------- 
	// Field Room

	/*
	* �ʵ� �̵� ��û
	*
	*	WORD	Type
	*	DWORD	RoomNo					// �̵� �� �ʵ�
	*
	*/
	en_PACKET_CS_LOBBY_REQ_MOVE_FIELD,
	/*
	* �ʵ� �̵� ����
	*
	*	WORD	Type
	*	BYTE	Status					// �̵� ��û ��� (�Ʒ� ���� �Ǿ� �ֽ��ϴ�.)
	*	DWORD	roomNo
	*	char	fieldName[20]
	*
	*/
	en_PACKET_SC_LOBBY_RES_MOVE_FIELD,
	/*
	* Ŭ���ʿ��� �ʵ� �غ� �ƴٴ� ���� �˸�
	*
	*	WORD	Type
	*
	*/
	en_PACKET_CS_LOBBY_READY_FIELD,



	/*
	* ä�� �̵� ��û
	*
	*	WORD	Type
	*	char	IP[16]
	*	WORD	Port
	*
	*/
	en_PACKET_CS_LOBBY_REQ_MOVE_CHANNEL,
	/*
	* ä�� �̵� ����
	*
	*	WORD	Type
	*	char	SessionKey[64]		// token
	*
	*/
	en_PACKET_SC_LOBBY_RES_MOVE_CHANNEL,


	/*
	* ä�� ��� ��û
	*
	*	WORD	Type
	*
	*/
	en_PACKET_CS_LOBBY_REQ_CHANNEL_LIST,
	/*
	* ä�� ��� ����
	*
	*	WORD	Type
	*	DWORD64	ChannelCount
	*	// ChannelCount ��ŭ �����ϴ�.
	*	{
	*		char	name[20]
	*		char	IP[16]
	*		WORD	Port
	*		BYTE	Status		// ä���� �ο� ��Ȳ�� �˷��ݴϴ�. (�Ʒ��� ����)
	*	}
	*/
	en_PACKET_SC_LOBBY_RES_CHANNEL_LIST,


	//---------------------------------------------------------
	// ����

	/*
	* ���� ��ǰ ��� ��û
	*
	*	WORD	type
	*	BYTE	storeID
	*
	*/
	en_PACKET_CS_LOBBY_REQ_STORE_PRODUCT_LIST,
	/*
	* ���� ��ǰ ��� ����
	*
	*	WORD	Type
	*	DWORD64	productCount			// ��ǰ ��� ��
	*	// productCount ��ŭ �����ϴ�.
	*	{
	*		BYTE	itemType
	*		DWORD	itemID
	*		char	itemName[64]
	*		char	description[512]
	*
	*		BYTE	moneyID
	*		DWORD	moneyPrice
	*		BYTE	tokenID
	*		DWORD	tokenPrice
	*	}
	*/
	en_PACKET_SC_LOBBY_RES_STORE_PRODUCT_LIST,


	/*
	* ���� ������ ���� ��û
	*
	*	WORD	Type
	*	BYTE	StoreID			// ���� �ڵ�
	*	DWORD	ItemID			// ������ �ڵ�
	*	DWORD	quantity		// ���� ����
	*
	*/
	en_PACKET_CS_LOBBY_REQ_BUY_STORE_ITEM,
	/*
	* ���� ������ ���� ����
	*
	*	WORD	Type
	*	BYTE	Status					// ���� ��� (�Ʒ��� ����)
	*	BYTE	MoneyID
	*	DWORD	MoneyQuantity			// ���� ���߿� ���� ��ȭ
	*
	*	BYTE	tokenID
	*	DWORD64	tokenSlotUpdateCount
	*	// tokenSlotUpdateCount ��ŭ �����ϴ�.
	*	{
	*		DWORD	seq
	*		DOWRD	quantity
	*	}
	*
	*	BYTE	itemType				// item type
	*	DWORD	itemID
	*	char	itemName[64]
	*	char	description[512]
	*	DWORD	maxQuantity
	*	DWORD64	itemSlotUpdateCount	// ����� �κ��丮 ������ ��
	*	// itemSlotUpdateCount	 ��ŭ �����ϴ�.
	*	{
	*
	*		DWORD	seq
	*		DWORD	quantity
	*	}
	*
	*/
	en_PACKET_SC_LOBBY_RES_BUY_STORE_ITEM,


	/*
	* Show Me The Money ��û
	*
	*	WORD	Type
	*	BYTE	moneyID
	*
	*/
	en_PACKET_CS_LOBBY_REQ_SHOW_ME_THE_MONEY,
	/*
	* Show Me The Money ����
	*
	*	WORD	Type
	*	BYTE	moneyID
	*	DWORD	moneyQuantity
	*
	*/
	en_PACKET_SC_LOBBY_RES_SHOW_ME_THE_MONEY,


	/*
	* ������ ��� ��û
	*
	*	WORD	type
	*	DWORD	itemID
	*
	*/
	en_PACKET_CS_LOBBY_REQ_USE_ITEM,
	/*
	*
	*	WORD	type
	*	BYTE	status = false;			// ������ ��� ����
	*
	*/
	en_PACKET_SC_LOBBY_RES_USE_ITEM_FAILED,
	/*
	* ������ ��� ����
	*
	*	WORD	type
	*	BYTE	statsID
	*	DWORD	stats
	* 
	* 	BYTE	itemType				// ���� ������ ����
	*	DWORD	itemID
	*	char	itemName[64]
	*	char	description[512]
	*	DWORD	maxQuantity
	*	DWORD64	itemSlotUpdateCount	// ����� �κ��丮 ������ ��
	*	// itemSlotUpdateCount	 ��ŭ �����ϴ�.
	*	{
	*
	*		DWORD	seq
	*		DWORD	quantity
	*	}
	*
	*/
	en_PACKET_SC_LOBBY_RES_USE_ITEM_STATS_BOX,

	//---------------------------------------------------------

	/*
	* ����(��)�� �ٸ� ģ������ ģ�� �߰� ��û
	*
	*	WORD	Type
	*	char	Nickname[20]			// ģ�� nickname
	*
	*/
	en_PACKET_CS_LOBBY_REQ_ADD_FRIEND,
	/*
	*
	*	WORD	type
	*	BYTE	status
	*
	*/
	en_PACKET_SC_LOBBY_RES_ADD_FRIEND,



	/*
	* �ٸ� ĳ���Ͱ� ����(��)���� ģ�� �߰� ��û
	*
	*	WORD	Type
	*	char	SendNickname[20]		// ģ�� �߰��� ���� nickname
	*/
	en_PACKET_SC_LOBBY_REQ_REQ_FRIEND,


	/*
	* �ٸ� ĳ���Ͱ� ����(��)���� ģ�� �߰� ����
	*
	*	WORD	Type
	*	char	SendNickname[20]	// ģ�� �߰��� ���� nickname
	*/
	en_PACKET_SC_LOBBY_REQ_ACCEPT_FRIEND,


	/*
	* ģ�� �߰� ���� ��û
	*
	*	WORD	Type
	*	char	Nickname[20]			// �¶��� ģ�� nickname
	*
	*/
	en_PACKET_CS_LOBBY_REQ_ACCEPT_ADD_FRIEND,
	/*
	* ģ�� �߰� ���� ���� (ģ�� �߰� ������ ���� �������� ����)
	*
	*	WORD	Type
	*	BYTE	Status					// ���
	*	char	Nickname[20]			// �¶��� ģ�� nickname
	*
	*/
	en_PACKET_SC_LOBBY_RES_ACCEPT_ADD_FRIEND,


	/*
	* ģ�� ��� ��û
	*
	*	WORD	Type
	*
	*/
	en_PACKET_CS_LOBBY_REQ_FRIEND_LIST,
	/*
	* ģ�� ��� ����
	*
	*	WORD	Type
	*	DWORD64	FriendCount				// ģ�� ��� ��
	*	// FriendCount ��ŭ �����ϴ�.
	*	{
	*		char	FriendNickname[20]	// ģ�� �г���
	*		BYTE	ConnectionStatus	// �α��� ����
	*	}
	*/
	en_PACKET_SC_LOBBY_RES_FRIEND_LIST,


	/*
	* �ӼӸ� �۽� ��û
	*
	*	WORD	Type
	*	char	RecvNickname[20]
	*	char	Chatting[512]
	*
	*/
	en_PACKET_CS_LOBBY_REQ_SEND_WHISPER,
	/*
	* �ӼӸ� �۽� ����
	*
	*	WORD	Type
	*	BYTE	Status
	*
	*/
	en_PACKET_SC_LOBBY_RES_SEND_WHISPER,

	/*
	* �ӼӸ� �۽�
	*
	*	WORD	Type
	*	char	SendNickname[20]
	*	char	Chatting[512]
	*
	*/
	en_PACKET_SC_LOBBY_REQ_SEND_WHISPER


};

enum class en_PACKET_STATUS_BUY_STORE_ITEM : BYTE
{
	SUCCESS,
	LACK_MONEY,
	LACK_TOKEN,
	FALL_INVENTORY,
};

enum class en_PACKET_STATUS_USE_ITEM : BYTE
{
	LACK_ITEM,
};

#endif // __LOBBYSERVER_LOBBYPROTOCOL_H__
