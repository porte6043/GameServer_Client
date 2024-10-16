#ifndef __LOBBYSERVER_CHARACTERSTATS_H__
#define __LOBBYSERVER_CHARACTERSTATS_H__
#include <Windows.h>
#include <vector>
using std::vector;
#include <map>
using std::map;
#include <string>
using std::string;


enum class en_TYPE_CHARACTER_STATS : BYTE
{
	DUMMY = 0,
	STR,
	DEX,
	INT,
	LUK
};



class CStats
{
public:
	struct st_Stats
	{
		BYTE	id;
		DWORD	quantity;
		string	name;
	};

private:
	map<BYTE, DWORD> stats;

public:
	void setStats(const BYTE& statsID, const DWORD& quantity);

	DWORD addStats(const BYTE& statsID, const DWORD& addQuantity);

	st_Stats getStats(const BYTE& statsID) const;
	
	vector<st_Stats> getStatsList() const;

	DWORD64	size() const;

	void clear();
};
#endif