#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <map>
using std::string;
using std::vector;
using std::map;

#include "Item.h"
#include "Money.h"
#include "Stats.h"

struct st_Character
{
	DWORD64			sessionID;
	DWORD64			uid;

	// 캐릭터 위치
	DWORD			roomNo;
	string			fieldName;

	// 캐릭터 이름
	string			nickname;

	// 능력치
	map<BYTE, st_Stats> stats;

	// 돈
	map<BYTE, st_Money> moneys;

	map<DWORD, vector<st_Item>> items;	// {itemType, inventory}

	// 친구목록
	vector<string>	friends;
	// 친구 추가 목록
	vector<string>	addFriends;

	void clear()
	{
		nickname.clear();
		stats.clear();
		moneys.clear();
		items.clear();
		friends.clear();
		addFriends.clear();
	}
};
