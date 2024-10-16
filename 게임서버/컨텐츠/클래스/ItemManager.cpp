#include "ItemManager.h"

// ���̺귯��
#include "���� ���̺귯��/CCsvParser.h"
#include "���� ���̺귯��/SerialRingBuffer.h"

// ����
#include "CBaseField.h"

// ����ü
#include "Character.h"
#include "Money.h"

// �����Լ�
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
		// { ������ �з�, ������ ����, �����۸�, ������ID, ������ ����, �ִ� ����}
		CCsvParser CsvParser("������ ����Ʈ.csv");
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
		// �Ϲ� ������(�Һ�)
		return useNomalItem(itemID, user);
	}
	else if (itemID <= 1999)
	{
		// ��Ű�� ������
		return usePackageItem(itemID, user);
	}
	else if (itemID <= 2999)
	{
		// ĳ�� ������
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
		// �Ϲ� ������
	}
	else if (itemID <= 666)
	{
		// �Һ� ������
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
		// ��� ������
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

	// ������ ��� ���� üũ
	if (user->inventory.getItemQuantity(item) == 0)
	{
		// ���������ʴ� ������
		user->disconnect();
		return false;
	}

	// ������ ��� ����
	DWORD Stats = user->stats.addStats(statsID, 2);
	auto invenUpdateInfo = user->inventory.removeItem(item).value();

	// ��� �ļ� ��ġ
	// DB ����
	vector<string> querys;
	querys.push_back(Query_Update_User_Stats(user->Nickname, statsID, Stats));
	invenUpdateInfo.getQuery(user->Nickname, querys);
	user->getField()->DB_Write.RequestTransactionQuery(querys, nullptr);

	// ���� �۽�
	CPacket packet;
	Packet_Init_Use_Item_StatsBox(packet, statsID, Stats, item, invenUpdateInfo);
	user->sendMassage(packet);

	return true;
}
bool CItemManager::execute_DexBox(const DWORD& itemID, st_User* user)
{
	auto item = getItem(itemID).value(); item.setQuantity(1);
	BYTE statsID = static_cast<BYTE>(en_TYPE_CHARACTER_STATS::DEX);

	// ������ ��� ���� üũ
	if (user->inventory.getItemQuantity(item) == 0)
	{
		// ���������ʴ� ������
		user->disconnect();
		return false;
	}

	// ������ ��� ����
	DWORD Stats = user->stats.addStats(statsID, 2);
	auto invenUpdateInfo = user->inventory.removeItem(item).value();

	// ��� �ļ� ��ġ
	// DB ����
	vector<string> querys;
	querys.push_back(Query_Update_User_Stats(user->Nickname, statsID, Stats));
	invenUpdateInfo.getQuery(user->Nickname, querys);
	user->getField()->DB_Write.RequestTransactionQuery(querys, nullptr);

	// ���� �۽�
	CPacket packet;
	Packet_Init_Use_Item_StatsBox(packet, statsID, Stats, item, invenUpdateInfo);
	user->sendMassage(packet);

	return true;
}
bool CItemManager::execute_IntBox(const DWORD& itemID, st_User* user)
{
	auto item = getItem(itemID).value(); item.setQuantity(1);
	BYTE statsID = static_cast<BYTE>(en_TYPE_CHARACTER_STATS::INT);

	// ������ ��� ���� üũ
	if (user->inventory.getItemQuantity(item) == 0)
	{
		// ���������ʴ� ������
		user->disconnect();
		return false;
	}

	// ������ ��� ����
	DWORD Stats = user->stats.addStats(statsID, 2);
	auto invenUpdateInfo = user->inventory.removeItem(item).value();

	// ��� �ļ� ��ġ
	// DB ����
	vector<string> querys;
	querys.push_back(Query_Update_User_Stats(user->Nickname, statsID, Stats));
	invenUpdateInfo.getQuery(user->Nickname, querys);
	user->getField()->DB_Write.RequestTransactionQuery(querys, nullptr);

	// ���� �۽�
	CPacket packet;
	Packet_Init_Use_Item_StatsBox(packet, statsID, Stats, item, invenUpdateInfo);
	user->sendMassage(packet);

	return true;
}
bool CItemManager::execute_LukBox(const DWORD& itemID, st_User* user)
{
	auto item = getItem(itemID).value(); item.setQuantity(1);
	BYTE statsID = static_cast<BYTE>(en_TYPE_CHARACTER_STATS::INT);


	// ������ ��� ���� üũ
	if (user->inventory.getItemQuantity(item) == 0)
	{
		// ���������ʴ� ������
		user->disconnect();
		return false;
	}

	// ������ ��� ����
	DWORD Stats = user->stats.addStats(statsID, 2);
	auto invenUpdateInfo = user->inventory.removeItem(item).value();

	// ��� �ļ� ��ġ
	// DB ����
	vector<string> querys;
	querys.push_back(Query_Update_User_Stats(user->Nickname, statsID, Stats));
	invenUpdateInfo.getQuery(user->Nickname, querys);
	user->getField()->DB_Write.RequestTransactionQuery(querys, nullptr);

	// ���� �۽�
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