#include "StoreManager.h"

// ���̺귯��
#include "���� ���̺귯��/CCsvParser.h"
#include "���� ���̺귯��/Log.h"
#include "ItemManager.h"

// ����ü
#include "Character.h"
#include "Money.h"

// ����
#include "CBaseField.h"

// ���� �Լ�
#include "Query.h"
#include "PacketInit.h"

map<BYTE, map<BYTE, st_Product>> StoreProductMap;


bool CStoreManager::buy(const BYTE& storeID, const DWORD& itemID, const DWORD& buyQuantity, st_User* user)
{	
	// ���� Ȯ��
	auto optProduct = getStoreProduct(storeID, itemID);
	if (!optProduct.has_value())
	{
		user->disconnect();
		return false;
	}
	st_Product product = optProduct.value();

	// ������ Ȯ��
	CItemManager itemManager;
	auto opt_item = itemManager.getItem(product.itemID);
	if (!opt_item.has_value())
	{
		user->disconnect();
		return false;
	}
	st_Item item = opt_item.value().setQuantity(buyQuantity);

	// ���� ���� üũ
	if(product.moneyID != 0)
	{
		// ��ȭ ����
		DWORD userMoney = user->inventory.getMoney(product.moneyID);
		if (userMoney < product.moneyPrice * buyQuantity)
		{
			// ���� ��ȭ ����
			CPacket packet;
			Packet_Init_Buy_Store_Item(packet, static_cast<BYTE>(en_PACKET_STATUS_BUY_STORE_ITEM::LACK_MONEY));
			user->sendMassage(packet);
			return false;
		}

		if (!user->inventory.isAddItem(item))
		{
			// �κ��丮 ����
			CPacket packet;
			Packet_Init_Buy_Store_Item(packet, static_cast<BYTE>(en_PACKET_STATUS_BUY_STORE_ITEM::FALL_INVENTORY));
			user->sendMassage(packet);
			return false;
		}


		// ��ȭ, ������ �޸� ����
		DWORD finalMoney = user->inventory.subMoney(product.moneyID, product.moneyPrice * buyQuantity);
		auto optInventoryUpdateInfo = user->inventory.addItem(item);
		
		// ��ȭ, ������ DB ����
		vector<string> querys;
		querys.push_back(Query_Update_User_Money(user->Nickname, product.moneyID, finalMoney));
		optInventoryUpdateInfo.value().getQuery(user->Nickname, querys);
		user->getField()->DB_Write.RequestTransactionQuery(querys, nullptr);

		// user ����
		CPacket packet;
		Packet_Init_Buy_Store_Item(
			packet, static_cast<BYTE>(en_PACKET_STATUS_BUY_STORE_ITEM::SUCCESS),
			product.moneyID, finalMoney,
			product.tokenID, {},
			item, optInventoryUpdateInfo.value());
		user->sendMassage(packet);
		return true;
	}
	else if (product.tokenID != 0)
	{
		// Token ����
		st_Item token = itemManager.getItem(product.tokenID).value().setQuantity(product.tokenPrice);
		DWORD tokenQuantity = user->inventory.getItemQuantity(token);
		if (tokenQuantity < product.tokenPrice)
		{
			// ���� ��ū ����
			CPacket packet;
			Packet_Init_Buy_Store_Item(packet, static_cast<BYTE>(en_PACKET_STATUS_BUY_STORE_ITEM::LACK_TOKEN));
			user->sendMassage(packet);
			return false;
		}

		if (!user->inventory.isAddItem(item))
		{
			// �κ��丮 ����
			CPacket packet;
			Packet_Init_Buy_Store_Item(packet, static_cast<BYTE>(en_PACKET_STATUS_BUY_STORE_ITEM::FALL_INVENTORY));
			user->sendMassage(packet);
			return false;
		}
		
		// ��ū, ������ �޸� ����
		auto tokenSlotUpdateInfo = user->inventory.removeItem(token).value();
		auto itemSlotUpdateInfo = user->inventory.addItem(item).value();
			
		// ��ū, ������ DB ����
		vector<string> querys;
		tokenSlotUpdateInfo.getQuery(user->Nickname, querys);
		itemSlotUpdateInfo.getQuery(user->Nickname, querys);
		user->getField()->DB_Write.RequestTransactionQuery(querys, nullptr);

		// user ����
		CPacket packet;
		Packet_Init_Buy_Store_Item(
			packet, static_cast<BYTE>(en_PACKET_STATUS_BUY_STORE_ITEM::SUCCESS),
			product.moneyID, 0,
			product.tokenID, tokenSlotUpdateInfo,
			item, itemSlotUpdateInfo);
		user->sendMassage(packet);
		return true;
	}
	else
	{
		LOG(L"server_StoreMananger", CLog::LEVEL_ERROR, L"������ ������ ��Ʈ ����");
		return false;
	}
}


bool CStoreManager::sell(const DWORD& itemID, const DWORD& quantity, st_User* user)
{
	// �̱���
	// ���� ��� 
	// 1. item ���� ���̺�( + st_Item)�� itemID�� ���� �Ǹ� ���� ����
	// 2. itemID�� ������ �ͼ� �ݿ��մϴ�.
}



vector<st_Product> CStoreManager::getStoreList(const BYTE& StoreCode)
{
	auto iter_StoreProductMap = StoreProductMap.find(StoreCode);
	if (iter_StoreProductMap == StoreProductMap.end())
		return {};

	auto storeProductList = iter_StoreProductMap->second;
	vector<st_Product> vec(storeProductList.size());
	int idx = 0;
	for (auto iter_ProductMap = storeProductList.begin(); iter_ProductMap != storeProductList.end(); ++iter_ProductMap)
	{
		vec[idx++] = iter_ProductMap->second;
	}

	return vec;
}

bool CStoreManager::setStoreList()
{
	static bool setFlag = false;

	if(!setFlag)
	{
		// { ����ID, ������ID, ��ȭID, ��ȭ ����, ��ūID, ��ū ��ȭ ���� }
		CCsvParser CsvParser("���� ����Ʈ.csv");
		auto data = CsvParser.Parse();

		for (int idx = 1; idx < data.size(); ++idx)
		{
			auto& row = data[idx];
			BYTE StoreCode = std::stoi(row[0]);
			BYTE ItemCode = std::stoi(row[1]);
			st_Product Product = { row[1], row[2], row[3], row[4], row[5] };
			StoreProductMap[StoreCode][ItemCode] = Product;
		}

		return true;
	}

	return false;
}

optional<st_Product> CStoreManager::getStoreProduct(const BYTE& storeID, const DWORD& ItemID)
{
	auto iter_StoreProductMap = StoreProductMap.find(storeID);
	if (iter_StoreProductMap == StoreProductMap.end())
		return std::nullopt;

	auto ProductMap = iter_StoreProductMap->second;
	auto iter_ProductMap = ProductMap.find(ItemID);
	if (iter_ProductMap == ProductMap.end())
		return std::nullopt;

	return iter_ProductMap->second;
}