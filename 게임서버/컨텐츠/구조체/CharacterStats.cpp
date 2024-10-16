#include "CharacterStats.h"

void CStats::setStats(const BYTE& statsID, const DWORD& quantity)
{
	stats[statsID] = quantity;
}

DWORD CStats::addStats(const BYTE& statsID, const DWORD& addQuantity)
{
	stats[statsID] += addQuantity;
	return stats[statsID];
}


CStats::st_Stats CStats::getStats(const BYTE& statsID) const
{	
	switch (static_cast<en_TYPE_CHARACTER_STATS>(statsID))
	{
	case en_TYPE_CHARACTER_STATS::STR:
		return { statsID, stats.at(statsID), "str"};
	case en_TYPE_CHARACTER_STATS::DEX:
		return { statsID, stats.at(statsID), "dex" };
	case en_TYPE_CHARACTER_STATS::INT:
		return { statsID, stats.at(statsID), "int" };
	case en_TYPE_CHARACTER_STATS::LUK:
		return { statsID, stats.at(statsID), "luk" };
	default:
		return { static_cast<BYTE>(en_TYPE_CHARACTER_STATS::DUMMY), 0, "" };
	}
}

vector<CStats::st_Stats> CStats::getStatsList() const
{
	vector<st_Stats> statsList;

	for (auto& Stats : stats)
	{
		statsList.push_back(getStats(Stats.first));
	}

	return statsList;
}

DWORD64	CStats::size() const
{
	return stats.size();
}


void CStats::clear()
{
	stats.clear();
}