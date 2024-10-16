#ifndef __LOBBYSERVER_LOBBYPROTOCOL_H__
#define __LOBBYSERVER_LOBBYPROTOCOL_H__



enum en_PACEKT_LOBBY_TYPE : short
{
	/*
	* 로비 서버 로그인 요청
	*
	*	WORD	Type
	*	INT64	UID
	*	char	SessionKey[64]
	*
	*/
	en_PACKET_CS_LOBBY_REQ_LOGIN = 0,
	/*
	* 로비 서버 로그인 응답
	*
	*	WORD	Type
	*	DWORD64	CharaterCount			// 보내 줄 캐릭터 수
	*	// 아래 데이터는 캐릭터 수 만큼 보냅니다.
	*	{
	*		char	Nickname[20]		// 캐릭터 닉네임 (null 포함)
	*		BYTE	CharacterSeq		// 캐릭터 순서
	*	}
	*
	*/
	en_PACKET_SC_LOBBY_RES_LOGIN,

	/*
	* 로비 서버 채널 이동 수락 요청
	*
	*	WORD	Type
	*	INT64	UID
	*	char	SessionKey[64]
	*	char	CharacterName[20]
	*
	*/
	en_PACKET_CS_LOBBY_REQ_LOGIN_OTHER_CHANNEL,
	/*
	* 로비 서버 채널 이동 수락 응답
	*
	*	WORD	Type
	*
	*/
	en_PACKET_SC_LOBBY_RES_LOGIN_OTHER_CHANNEL,


	/*
	* 선택 캐릭터 입장 요청
	*
	*	WORD	Type
	*	char	CharacterName[20]
	*
	*/
	en_PACKET_CS_LOBBY_REQ_SELECT_CHARACTER,
	/*
	* 로비 서버 선택 캐릭터 입장 응답
	*
	*	WORD	Type
	*	char	Nickname[20]			// 캐릭터 닉네임 (null 포함)
	*	DWORD64	uid
	*	DWORD	RoomNo					// 캐릭터 위치
	*
	*	DWORD64	statsCount				// 캐릭터 능력치
	*	// statsCount 만큼 보냅니다.
	*	{
	*		BYTE	statsID
	*		DWORD	quantity
	*		char	statsName[20]
	*	}
	*
	*	DWORD64	moneyCount
	*	// moneyCount 만큼 보냅니다.
	*	{
	*		BYTE	moneyID
	*		DWORD	quantity
	*		char	moneyName[20]
	*	}
	*
	*	DWORD	maxSlot					// 아이템 슬롯 갯수
	*	DWORD64	ItemCount				// 아이템 수
	*	// ItemCount 만큼 보냅니다.
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
	*	DWORD64	FriendCount				// 친구 목록 수
	*	// FriendCount 만큼 보냅니다.
	*	{
	*		char	FriendNickname[20]	// 친구 닉네임
	*	}
	*
	*	DWORD64	AddFriendCount			// 친구 목록 수
	*	// AddFriendCount 만큼 보냅니다.
	*	{
	*		char	FriendNickname[20]	// 친구 닉네임
	*	}
	*
	*/
	en_PACKET_SC_LOBBY_RES_SELECT_CHARACTER,

	/*
	* 캐릭터 리스트 요청
	*
	*	WORD	type
	*
	*/
	en_PACKET_CS_LOBBY_REQ_CHARACTERLIST,
	/*
	* 캐릭터 리스트 응답
	*
	*	WORD	type
	*	DWORD64	CharaterCount			// 보내 줄 캐릭터 수
	*	// 아래 데이터는 캐릭터 수 만큼 보냅니다.
	*	{
	*		char	Nickname[20]		// 캐릭터 닉네임 (null 포함)
	*		BYTE	CharacterSeq		// 캐릭터 순서
	*	}
	*
	*/
	en_PACKET_SC_LOBBY_RES_CHARACTERLIST,


	/*
	* 로비 서버 사용 가능한 캐릭터 이름 확인 요청
	*
	*	WORD	Type
	*	char	Nickname[20]
	*
	*/
	en_PACKET_CS_LOBBY_REQ_AVAILABLE_NICKNAME,
	/*
	* 로비 서버 사용 가능한 캐릭터 이름 확인 응답
	*
	*	WORD	Type
	*	BYTE	Status					// (true, false)
	*
	*/
	en_PACKET_SC_LOBBY_RES_AVAILABLE_NICKNAME,


	/*
	* 로비 서버 캐릭터 생성 요청
	*
	*	WORD	Type
	*	UID		UID
	*	char	Nickname[20]			// 생성할 캐릭터 이름
	*	WORD	CharacterSeq			// 생성할 캐릭터 순서
	*
	*/
	en_PACKET_CS_LOBBY_REQ_CREATE_CHARACTER,
	/*
	* 로비 서버 캐릭터 생성 응답
	*
	*	WORD	Type
	*	BYTE	Status					// 생성 가능 여부	 (아래 정의 되어 있습니다.)
	*
	*/
	en_PACKET_SC_LOBBY_RES_CREATE_CHARACTER,

	// Lobby Room
	//-------------------------------------------------------------- 
	// Field Room

	/*
	* 필드 이동 요청
	*
	*	WORD	Type
	*	DWORD	RoomNo					// 이동 할 필드
	*
	*/
	en_PACKET_CS_LOBBY_REQ_MOVE_FIELD,
	/*
	* 필드 이동 응답
	*
	*	WORD	Type
	*	BYTE	Status					// 이동 요청 결과 (아래 정의 되어 있습니다.)
	*	DWORD	roomNo
	*	char	fieldName[20]
	*
	*/
	en_PACKET_SC_LOBBY_RES_MOVE_FIELD,
	/*
	* 클라쪽에서 필드 준비가 됐다는 것을 알림
	*
	*	WORD	Type
	*
	*/
	en_PACKET_CS_LOBBY_READY_FIELD,



	/*
	* 채널 이동 요청
	*
	*	WORD	Type
	*	char	IP[16]
	*	WORD	Port
	*
	*/
	en_PACKET_CS_LOBBY_REQ_MOVE_CHANNEL,
	/*
	* 채널 이동 응답
	*
	*	WORD	Type
	*	char	SessionKey[64]		// token
	*
	*/
	en_PACKET_SC_LOBBY_RES_MOVE_CHANNEL,


	/*
	* 채널 목록 요청
	*
	*	WORD	Type
	*
	*/
	en_PACKET_CS_LOBBY_REQ_CHANNEL_LIST,
	/*
	* 채널 목록 응답
	*
	*	WORD	Type
	*	DWORD64	ChannelCount
	*	// ChannelCount 만큼 보냅니다.
	*	{
	*		char	name[20]
	*		char	IP[16]
	*		WORD	Port
	*		BYTE	Status		// 채널의 인원 상황을 알려줍니다. (아래에 정의)
	*	}
	*/
	en_PACKET_SC_LOBBY_RES_CHANNEL_LIST,


	//---------------------------------------------------------
	// 상점

	/*
	* 상점 물품 목록 요청
	*
	*	WORD	type
	*	BYTE	storeID
	*
	*/
	en_PACKET_CS_LOBBY_REQ_STORE_PRODUCT_LIST,
	/*
	* 상점 물품 목록 응답
	*
	*	WORD	Type
	*	DWORD64	productCount			// 상품 목록 수
	*	// productCount 만큼 보냅니다.
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
	* 상점 아이템 구매 요청
	*
	*	WORD	Type
	*	BYTE	StoreID			// 상점 코드
	*	DWORD	ItemID			// 아이템 코드
	*	DWORD	quantity		// 구매 수량
	*
	*/
	en_PACKET_CS_LOBBY_REQ_BUY_STORE_ITEM,
	/*
	* 상점 아이템 구매 응답
	*
	*	WORD	Type
	*	BYTE	Status					// 구매 결과 (아래에 정의)
	*	BYTE	MoneyID
	*	DWORD	MoneyQuantity			// 최종 수중에 남은 재화
	*
	*	BYTE	tokenID
	*	DWORD64	tokenSlotUpdateCount
	*	// tokenSlotUpdateCount 만큼 보냅니다.
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
	*	DWORD64	itemSlotUpdateCount	// 변경된 인벤토리 아이템 수
	*	// itemSlotUpdateCount	 만큼 보냅니다.
	*	{
	*
	*		DWORD	seq
	*		DWORD	quantity
	*	}
	*
	*/
	en_PACKET_SC_LOBBY_RES_BUY_STORE_ITEM,


	/*
	* Show Me The Money 요청
	*
	*	WORD	Type
	*	BYTE	moneyID
	*
	*/
	en_PACKET_CS_LOBBY_REQ_SHOW_ME_THE_MONEY,
	/*
	* Show Me The Money 응답
	*
	*	WORD	Type
	*	BYTE	moneyID
	*	DWORD	moneyQuantity
	*
	*/
	en_PACKET_SC_LOBBY_RES_SHOW_ME_THE_MONEY,


	/*
	* 아이템 사용 요청
	*
	*	WORD	type
	*	DWORD	itemID
	*
	*/
	en_PACKET_CS_LOBBY_REQ_USE_ITEM,
	/*
	*
	*	WORD	type
	*	BYTE	status = false;			// 아이템 사용 실패
	*
	*/
	en_PACKET_SC_LOBBY_RES_USE_ITEM_FAILED,
	/*
	* 아이템 사용 응답
	*
	*	WORD	type
	*	BYTE	statsID
	*	DWORD	stats
	* 
	* 	BYTE	itemType				// 사용된 아이템 갱신
	*	DWORD	itemID
	*	char	itemName[64]
	*	char	description[512]
	*	DWORD	maxQuantity
	*	DWORD64	itemSlotUpdateCount	// 변경된 인벤토리 아이템 수
	*	// itemSlotUpdateCount	 만큼 보냅니다.
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
	* 유저(내)가 다른 친구에게 친구 추가 요청
	*
	*	WORD	Type
	*	char	Nickname[20]			// 친구 nickname
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
	* 다른 캐릭터가 유저(나)에게 친구 추가 요청
	*
	*	WORD	Type
	*	char	SendNickname[20]		// 친구 추가를 보낸 nickname
	*/
	en_PACKET_SC_LOBBY_REQ_REQ_FRIEND,


	/*
	* 다른 캐릭터가 유저(나)에게 친구 추가 승인
	*
	*	WORD	Type
	*	char	SendNickname[20]	// 친구 추가를 보낸 nickname
	*/
	en_PACKET_SC_LOBBY_REQ_ACCEPT_FRIEND,


	/*
	* 친구 추가 승인 요청
	*
	*	WORD	Type
	*	char	Nickname[20]			// 승락한 친구 nickname
	*
	*/
	en_PACKET_CS_LOBBY_REQ_ACCEPT_ADD_FRIEND,
	/*
	* 친구 추가 승인 응답 (친구 추가 승인을 받은 유저에게 전송)
	*
	*	WORD	Type
	*	BYTE	Status					// 결과
	*	char	Nickname[20]			// 승락한 친구 nickname
	*
	*/
	en_PACKET_SC_LOBBY_RES_ACCEPT_ADD_FRIEND,


	/*
	* 친구 목록 요청
	*
	*	WORD	Type
	*
	*/
	en_PACKET_CS_LOBBY_REQ_FRIEND_LIST,
	/*
	* 친구 목록 응답
	*
	*	WORD	Type
	*	DWORD64	FriendCount				// 친구 목록 수
	*	// FriendCount 만큼 보냅니다.
	*	{
	*		char	FriendNickname[20]	// 친구 닉네임
	*		BYTE	ConnectionStatus	// 로그인 여부
	*	}
	*/
	en_PACKET_SC_LOBBY_RES_FRIEND_LIST,


	/*
	* 귓속말 송신 요청
	*
	*	WORD	Type
	*	char	RecvNickname[20]
	*	char	Chatting[512]
	*
	*/
	en_PACKET_CS_LOBBY_REQ_SEND_WHISPER,
	/*
	* 귓속말 송신 응답
	*
	*	WORD	Type
	*	BYTE	Status
	*
	*/
	en_PACKET_SC_LOBBY_RES_SEND_WHISPER,

	/*
	* 귓속말 송신
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
