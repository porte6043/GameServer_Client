#ifndef __LOBBYSERVER_QUERY_H__
#define __LOBBYSERVER_QUERY_H__
#include <Windows.h>
#include <string>
using std::string;

#include "Aliases.h"
struct st_Stats;
enum class en_ROOM_NUMBER : unsigned int;

/*
DB 스키마 이름					= world_korea

유저 목록 테이블					= account

캐릭터 목록 테이블				= character_list

캐릭터 능력치 테이블				= character_status

캐릭터 위치 테이블				= character_position

캐릭터 재화 테이블				= character_money

캐릭터 소비 아이템 목록 테이블		= character_consumable_item_list

캐릭터 친구 목록 테이블			= character_friend_list

캐릭터 친구추가 목록 테이블		= character_addfriend_list
*/

string Query_Select_CharacterList(const DWORD64& UID);

string Query_Select_Character(const string& CharacterName);

string Query_Select_Nickname(const string& CharacterName);

string Query_Insert_CreateCharacter(const UID& uid, const string& CharacterName, const WORD& CharaterSeq);
string Quert_Insert_CreateCharacter_Stats(const string& nickname);
string Query_Insert_CreateCharacter_Money(const string& CharacterName);
string Query_Insert_CreateCharacter_Position(const string& CharacterName);

string Query_Insert_AddFriendList(const string& RecvNickname, const string& SendNickname);
string Query_Delete_AddFriendList(const string& Nickname, const string& AddFriendNickname);

string Query_Insert_FriendList(const string& Nickname, const string& AcceptedNickname);
string Query_Delete_FriendList(const string& Nickname, const string& FrinendNickname);


//-----------------------------
// user 관련 쿼리

string Query_Update_User_RoomNo(const string& nickname, const en_ROOM_NUMBER& roomNo);
string Query_Update_User_Stats(const string& nickname, const BYTE& statsID, const DWORD& stats);
string Query_Update_User_Money(const string& nickname, const BYTE& moneyID, const DWORD& amount);

string Query_Insert_User_Item(const string& nickname, const DWORD& itemID, const DWORD& quantity, const DWORD& seq);
string Query_Update_User_Item(const string& nickname, const DWORD& itemID, const DWORD& quantity, const DWORD& seq);
string Query_Delete_User_Item(const string& nickname, const DWORD& seq);





#endif // __LOBBYSERVER_QUERY_H__
