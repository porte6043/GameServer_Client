#include "ItemManager.h"

// 라이브러리
#include "공용 라이브러리/CCsvParser.h"
#include "공용 라이브러리/SerialRingBuffer.h"

// 서버
#include "CBaseField.h"

// 구조체
#include "Character.h"
#include "Money.h"

// 전역함수
#include "PacketInit.h"
#include "Query.h"


map<DWORD, const st_Item> ItemMap;

optional<st_Item> CItemManager::getItem(const DWORD& itemID)
{
	auto iter_ItemMap = ItemMap.find(itemID);
	if (iter_ItemMap == ItemMap.end())
		return std::nullopt;

	return iter_ItemMap->second;
}
optional<reference_wrapper<const st_Item>> CItemManager::getItemByRef(const DWORD& itemID)
{
	auto iter_ItemMap = ItemMap.find(itemID);
	if (iter_ItemMap == ItemMap.end())
		return std::nullopt;

	return iter_ItemMap->second;
}

bool CItemManager::setItemList()
{
	static bool setFlag = false;

	if (!setFlag)
	{
		// { 아이템 분류, 아이템 종류, 아이템명, 아이템ID, 아이템 설명, 최대 개수}
		CCsvParser CsvParser("아이템 리스트.csv");
		auto data = CsvParser.Parse();

		for (int idx = 1; idx < data.size(); ++idx)
		{
			auto& row = data[idx];
			DWORD itemID = std::stoi(row[3]);
			st_Item item = { row[0], row[1], row[2], row[3], row[4], row[5] };
			ItemMap.insert({ itemID, item });
		}

		return true;
	}

	return false;
}

bool CItemManager::useItem(const DWORD& itemID, st_User* user)
{
	if (itemID <= 999)
	{
		// 일반 아이템(소비)
		return useNomalItem(itemID, user);
	}
	else if (itemID <= 1999)
	{
		// 패키지 아이템
		return usePackageItem(itemID, user);
	}
	else if (itemID <= 2999)
	{
		// 캐시 아이템
		return useCashItem(itemID, user);
	}
	else
	{
		user->disconnect();
		return false;
	}
}

bool CItemManager::useNomalItem(const DWORD& itemID, st_User* user)
{
	// 1 ~ 999
	if (itemID <= 333)
	{
		// 일반 아이템
	}
	else if (itemID <= 666)
	{
		// 소비 아이템
		switch (itemID)
		{
		case 334:
			return execute_StrBox(itemID, user);
		case 335:
			return execute_DexBox(itemID, user);
		case 336:
			return execute_IntBox(itemID, user);
		case 337:
			return execute_LukBox(itemID, user);
		default:
			user->disconnect();
			return false;
		}
	}
	else if (itemID <= 999)
	{
		// 장비 아이템
	}
	else
	{
		user->disconnect();
		return false;
	}
}
bool CItemManager::usePackageItem(const DWORD& itemID, st_User* user)
{
	// 1000 ~ 1999
	switch (itemID)
	{
	case 10:
		return execute_AllBox(itemID, user);
	case 11:
		return execute_StrBoxAndCash(itemID, user);
	default:
		user->disconnect();
		return false;
	}
}
bool CItemManager::useCashItem(const DWORD& itemID, st_User* user)
{
	// 2000 ~ 2999
	switch (itemID)
	{
	case 20:
		return execute_Preminum_StrBox(itemID, user);
	case 21:
		return execute_Preminum_DexBox(itemID, user);
	case 22:
		return execute_Preminum_IntBox(itemID, user);
	case 23:
		return execute_Preminum_LukBox(itemID, user);
	default:
		user->disconnect();
		return false;
	}
}

// nomal item
bool CItemManager::execute_StrBox(const DWORD& itemID, st_User* user)
{
	auto item = getItem(itemID).value(); item.setQuantity(1);
	BYTE statsID = static_cast<BYTE>(en_TYPE_CHARACTER_STATS::STR);

	// 아이템 사용 조건 체크
	if (user->inventory.getItemQuantity(item) == 0)
	{
		// 존재하지않는 아이템
		user->disconnect();
		return false;
	}

	// 아이템 기능 실행
	DWORD Stats = user->stats.addStats(statsID, 2);
	auto invenUpdateInfo = user->inventory.removeItem(item).value();

	// 사용 후속 조치
	// DB 저장
	vector<string> querys;
	querys.push_back(Query_Update_User_Stats(user->Nickname, statsID, Stats));
	invenUpdateInfo.getQuery(user->Nickname, querys);
	user->getField()->DB_Write.RequestTransactionQuery(querys, nullptr);

	// 유저 송신
	CPacket packet;
	Packet_Init_Use_Item_StatsBox(packet, statsID, Stats, item, invenUpdateInfo);
	user->sendMassage(packet);

	return true;
}
bool CItemManager::execute_DexBox(const DWORD& itemID, st_User* user)
{
	auto item = getItem(itemID).value(); item.setQuantity(1);
	BYTE statsID = static_cast<BYTE>(en_TYPE_CHARACTER_STATS::DEX);

	// 아이템 사용 조건 체크
	if (user->inventory.getItemQuantity(item) == 0)
	{
		// 존재하지않는 아이템
		user->disconnect();
		return false;
	}

	// 아이템 기능 실행
	DWORD Stats = user->stats.addStats(statsID, 2);
	auto invenUpdateInfo = user->inventory.removeItem(item).value();

	// 사용 후속 조치
	// DB 저장
	vector<string> querys;
	querys.push_back(Query_Update_User_Stats(user->Nickname, statsID, Stats));
	invenUpdateInfo.getQuery(user->Nickname, querys);
	user->getField()->DB_Write.RequestTransactionQuery(querys, nullptr);

	// 유저 송신
	CPacket packet;
	Packet_Init_Use_Item_StatsBox(packet, statsID, Stats, item, invenUpdateInfo);
	user->sendMassage(packet);

	return true;
}
bool CItemManager::execute_IntBox(const DWORD& itemID, st_User* user)
{
	auto item = getItem(itemID).value(); item.setQuantity(1);
	BYTE statsID = static_cast<BYTE>(en_TYPE_CHARACTER_STATS::INT);

	// 아이템 사용 조건 체크
	if (user->inventory.getItemQuantity(item) == 0)
	{
		// 존재하지않는 아이템
		user->disconnect();
		return false;
	}

	// 아이템 기능 실행
	DWORD Stats = user->stats.addStats(statsID, 2);
	auto invenUpdateInfo = user->inventory.removeItem(item).value();

	// 사용 후속 조치
	// DB 저장
	vector<string> querys;
	querys.push_back(Query_Update_User_Stats(user->Nickname, statsID, Stats));
	invenUpdateInfo.getQuery(user->Nickname, querys);
	user->getField()->DB_Write.RequestTransactionQuery(querys, nullptr);

	// 유저 송신
	CPacket packet;
	Packet_Init_Use_Item_StatsBox(packet, statsID, Stats, item, invenUpdateInfo);
	user->sendMassage(packet);

	return true;
}
bool CItemManager::execute_LukBox(const DWORD& itemID, st_User* user)
{
	auto item = getItem(itemID).value(); item.setQuantity(1);
	BYTE statsID = static_cast<BYTE>(en_TYPE_CHARACTER_STATS::INT);


	// 아이템 사용 조건 체크
	if (user->inventory.getItemQuantity(item) == 0)
	{
		// 존재하지않는 아이템
		user->disconnect();
		return false;
	}

	// 아이템 기능 실행
	DWORD Stats = user->stats.addStats(statsID, 2);
	auto invenUpdateInfo = user->inventory.removeItem(item).value();

	// 사용 후속 조치
	// DB 저장
	vector<string> querys;
	querys.push_back(Query_Update_User_Stats(user->Nickname, statsID, Stats));
	invenUpdateInfo.getQuery(user->Nickname, querys);
	user->getField()->DB_Write.RequestTransactionQuery(querys, nullptr);

	// 유저 송신
	CPacket packet;
	Packet_Init_Use_Item_StatsBox(packet, statsID, Stats, item, invenUpdateInfo);
	user->sendMassage(packet);

	return true;
}


// packege item
bool CItemManager::execute_AllBox(const DWORD& itemID, st_User* user)
{
	return true;
}
bool CItemManager::execute_StrBoxAndCash(const DWORD& itemID, st_User* user)
{
	return true;
}


// cash item
bool CItemManager::execute_Preminum_StrBox(const DWORD& itemID, st_User* user)
{
	return true;
}
bool CItemManager::execute_Preminum_DexBox(const DWORD& itemID, st_User* user)
{
	return true;
}
bool CItemManager::execute_Preminum_IntBox(const DWORD& itemID, st_User* user)
{
	return true;
}
bool CItemManager::execute_Preminum_LukBox(const DWORD& itemID, st_User* user)
{
	return true;
}