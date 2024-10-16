#pragma once
#include <Windows.h>
#include <string>
using std::string;

struct st_Item
{
	BYTE	type;		// 아이템 종류 ex) 일반, 소비, 장비
	DWORD	itemID;
	string	name;
	string	description;
	DWORD	maxQuantity;
	DWORD	quantity;
	DWORD	seq;
};