#pragma once
#include <Windows.h>
#include <string>
using std::string;

struct st_Item
{
	BYTE	type;		// ������ ���� ex) �Ϲ�, �Һ�, ���
	DWORD	itemID;
	string	name;
	string	description;
	DWORD	maxQuantity;
	DWORD	quantity;
	DWORD	seq;
};