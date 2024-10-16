#pragma once
#include <Windows.h>
#include <string>
using std::string;

struct st_Money
{
	BYTE	id;
	DWORD	quantity;
	string	name;
};