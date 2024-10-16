#include "Query.h"

#include "Money.h"
#include "CharacterStats.h"

#include "CharacterStats.h"
#include "RoomNumber.h"
#include "en_LobbyServer.h"

/*
DB 스키마 이름					= world_korea

유저 목록 테이블					= account

캐릭터 목록 테이블				= character_list

캐릭터 능력치 테이블				= character_stats

캐릭터 위치 테이블				= character_position

캐릭터 재화 테이블				= character_money

캐릭터 소비 아이템 목록 테이블		= character_consumable_item

캐릭터 친구 목록 테이블			= character_friend_list

캐릭터 친구추가 목록 테이블		= character_addfriend_list
*/

#define LEN_QUERY 2048

string Query_Select_CharacterList(const DWORD64& UID)
{
	const char* format = R"(
	SELECT
		nickname, character_seq
	FROM
		world_korea.character_list
	WHERE
		uid = %d
    )";

	char query[LEN_QUERY];
	sprintf_s(query, sizeof(query), format, UID);

	return query;
}

string Query_Select_Character(const string& CharacterName)
{
	const char* format = R"(
	SELECT
	nickname,
		(SELECT roomno
		 FROM	world_korea.character_position as cp
		 WHERE	cp.nickname = cl.nickname) AS RoomNo,

		(SELECT GROUP_CONCAT(CONCAT(cs.stats_id, ':', cs.stats))
		 FROM	world_korea.character_stats as cs
		 WHERE	cs.nickname = cl.nickname) AS StatsList,
		
		(SELECT GROUP_CONCAT(CONCAT(cm.money_id, ':', cm.money))
		 FROM	world_korea.character_money as cm
		 WHERE	cm.nickname = cl.nickname) AS Money,

		IFNULL(
		(SELECT GROUP_CONCAT(CONCAT(ccil.item_id, ':', ccil.quantity, ':', ccil.sequence))
		 FROM	world_korea.character_consumable_item as ccil
		 WHERE	ccil.nickname = cl.nickname) , '') AS ItemList,

		IFNULL(
		(SELECT GROUP_CONCAT(cfl.friend_nickname)
		 FROM	world_korea.character_friend_list as cfl
		 WHERE	cfl.nickname = cl.nickname), '') AS FriendList,
		IFNULL(
		(SELECT GROUP_CONCAT(cafl.request_nickname)
		 FROM	world_korea.character_addfriend_list as cafl
		 WHERE	cafl.nickname = cl.nickname), '') AS AddFriendList
	FROM
		world_korea.character_list as cl
	WHERE
		nickname = '%s';
    )";

	char query[LEN_QUERY];
	sprintf_s(query, sizeof(query), format, CharacterName.c_str());

	return query;
}

string Query_Select_Nickname(const string& CharacterName)
{
	const char* format = R"(
	SELECT
		nickname, character_seq
	FROM
		world_korea.character_list
	WHERE
		nickname = '%s'
    )";

	char query[LEN_QUERY];
	sprintf_s(query, sizeof(query), format, CharacterName.c_str());

	return query;
}

string Query_Insert_CreateCharacter(const UID& uid, const string& CharacterName, const WORD& CharaterSeq)
{
	const char* format = R"(
	INSERT INTO
		world_korea.character_list (uid, nickname, character_seq)
	VALUES
		(%lld, '%s', %d)
    )";

	char query[LEN_QUERY];
	sprintf_s(query, sizeof(query), format, uid, CharacterName.c_str(), CharaterSeq);

	return query;
}
string Quert_Insert_CreateCharacter_Stats(const string& nickname)
{
	const char* format = R"(
	INSERT INTO
		world_korea.character_stats (nickname, stats_id, stats)
	VALUES
		('%s', %d, %d),
		('%s', %d, %d),
		('%s', %d, %d),
		('%s', %d, %d)
    )";

	char query[LEN_QUERY];
	sprintf_s(query, sizeof(query), format,
		nickname, en_TYPE_CHARACTER_STATS::STR, 10,
		nickname, en_TYPE_CHARACTER_STATS::DEX, 10,
		nickname, en_TYPE_CHARACTER_STATS::INT, 10,
		nickname, en_TYPE_CHARACTER_STATS::LUK, 10
		);

	return query;
}
string Query_Insert_CreateCharacter_Money(const string& CharacterName)
{
	const char* format = R"(
	INSERT INTO
		world_korea.character_money (nickname, money_id, money)
	VALUES
		('%s', %d, %d),
		('%s', %d, %d)
    )";

	char query[LEN_QUERY];
	sprintf_s(query, sizeof(query), format, 
		CharacterName.c_str(), en_TYPE_MONEY::MESO, 0,
		CharacterName.c_str(), en_TYPE_MONEY::CASH, 0
		);

	return query;
}
string Query_Insert_CreateCharacter_Position(const string& CharacterName)
{
	const char* format = R"(
	INSERT INTO
		world_korea.character_position (nickname, roomno, x, y)
	VALUES
		('%s', %d, %d, %d)
    )";

	char query[LEN_QUERY];
	sprintf_s(query, sizeof(query), format, CharacterName.c_str(), en_ROOM_NUMBER::Village1, 0, 0);

	return query;
}

string Query_Insert_AddFriendList(const string& RecvNickname, const string& SendNickname)
{
	const char* format = R"(
	INSERT INTO
		world_korea.character_addfriend_list (nickname, request_nickname)
	VALUES
		('%s', '%s')
    )";

	char query[LEN_QUERY];
	sprintf_s(query, sizeof(query), format, RecvNickname.c_str(), SendNickname.c_str());

	return query;
}

string Query_Delete_AddFriendList(const string& Nickname, const string& AddFriendNickname)
{
	const char* format = R"(
	DELETE FROM
		world_korea.character_addfriend_list
	WHERE
		nickname = '%s' AND request_nickname = '%s'
    )";

	char query[LEN_QUERY];
	sprintf_s(query, sizeof(query), format, Nickname.c_str(), AddFriendNickname.c_str());

	return query;
}

string Query_Insert_FriendList(const string& Nickname, const string& AcceptedNickname)
{
	const char* format = R"(
	INSERT INTO
		world_korea.character_friend_list (nickname, friend_nickname)
	VALUES
		('%s', '%s')
    )";

	char query[LEN_QUERY];
	sprintf_s(query, sizeof(query), format, Nickname.c_str(), AcceptedNickname.c_str());

	return query;
}

string Query_Delete_FriendList(const string& Nickname, const string& FrinendNickname)
{
	const char* format = R"(
	DELETE FROM
		world_korea.character_friend_list
	WHERE
		nickname = '%s' AND friend_nickname = '%s'
    )";

	char query[LEN_QUERY];
	sprintf_s(query, sizeof(query), format, Nickname.c_str(), FrinendNickname.c_str());

	return query;
}




//-----------------------------
// user 관련 쿼리
string Query_Update_User_RoomNo(const string& nickname, const en_ROOM_NUMBER& roomNo)
{
	const char* format = R"(
	UPDATE
		world_korea.character_position
	SET
		roomno = %d
	WHERE
		nickname = '%s'
    )";

	char query[LEN_QUERY];
	sprintf_s(query, sizeof(query), format, roomNo, nickname.c_str());

	return query;

}
string Query_Update_User_Stats(const string& nickname, const BYTE& statsID, const DWORD& stats)
{
	const char* format = R"(
	UPDATE
		world_korea.character_stats
	SET
		stats = %d
	WHERE
		nickname = '%s' AND stats_id = %d
    )";

	char query[LEN_QUERY];
	sprintf_s(query, sizeof(query), format, stats, nickname.c_str(), statsID);

	return query;
}
string Query_Update_User_Money(const string& nickname, const BYTE& moneyID, const DWORD& amount)
{
	const char* format = R"(
	UPDATE
		world_korea.character_money
	SET
		money = %d
	WHERE
		nickname = '%s' AND money_id = %d
    )";

	char query[LEN_QUERY];
	sprintf_s(query, sizeof(query), format, amount, nickname.c_str(), moneyID);

	return query;
}

string Query_Insert_User_Item(const string& nickname, const DWORD& itemID, const DWORD& quantity, const DWORD& seq)
{
	const char* format = R"(
	INSERT INTO
		world_korea.character_consumable_item (nickname, item_id, quantity, sequence)
	VALUES
		('%s', %d, %d, %d)
    )";

	char query[LEN_QUERY];
	sprintf_s(query, sizeof(query), format, nickname.c_str(), itemID, quantity, seq);

	return query;
}
string Query_Update_User_Item(const string& nickname, const DWORD& itemID, const DWORD& quantity, const DWORD& seq)
{
	const char* format = R"(
	UPDATE
		world_korea.character_consumable_item
	SET
		item_id = %d, quantity = %d
	WHERE
		nickname = '%s' AND sequence = %d
    )";

	char query[LEN_QUERY];
	sprintf_s(query, sizeof(query), format, itemID, quantity, nickname.c_str(), seq);

	return query;
}
string Query_Delete_User_Item(const string& nickname, const DWORD& seq)
{
	const char* format = R"(
	DELETE FROM
		world_korea.character_consumable_item
	WHERE
		nickname = '%s' AND sequence = %d
    )";

	char query[LEN_QUERY];
	sprintf_s(query, sizeof(query), format, nickname.c_str(), seq);

	return query;
}