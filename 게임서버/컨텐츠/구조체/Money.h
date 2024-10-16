#ifndef __LOBBYSERVER_MONEY_H__
#define __LOBBYSERVER_MONEY_H__
#include <Windows.h>
#include <string>
using std::string;

enum class en_TYPE_MONEY : BYTE
{
	DUMMY = 0,
	MESO,
	CASH
};

struct st_Money
{
	BYTE	moneyID;
	DWORD	quantity;
};

string getMoneyName(const BYTE& moneyId);
#endif