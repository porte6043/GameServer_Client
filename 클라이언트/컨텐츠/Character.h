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

	// ĳ���� ��ġ
	DWORD			roomNo;
	string			fieldName;

	// ĳ���� �̸�
	string			nickname;

	// �ɷ�ġ
	map<BYTE, st_Stats> stats;

	// ��
	map<BYTE, st_Money> moneys;

	map<DWORD, vector<st_Item>> items;	// {itemType, inventory}

	// ģ�����
	vector<string>	friends;
	// ģ�� �߰� ���
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
