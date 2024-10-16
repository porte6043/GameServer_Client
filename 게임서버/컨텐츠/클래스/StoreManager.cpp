#include "StoreManager.h"

// 라이브러리
#include "공용 라이브러리/CCsvParser.h"
#include "공용 라이브러리/Log.h"
#include "ItemManager.h"

// 구조체
#include "Character.h"
#include "Money.h"

// 서버
#include "CBaseField.h"

// 전역 함수
#include "Query.h"
#include "PacketInit.h"

map<BYTE, map<BYTE, st_Product>> StoreProductMap;


bool CStoreManager::buy(const BYTE& storeID, const DWORD& itemID, const DWORD& buyQuantity, st_User* user)
{	
	// 상인 확인
	auto optProduct = getStoreProduct(storeID, itemID);
	if (!optProduct.has_value())
	{
		user->disconnect();
		return false;
	}
	st_Product product = optProduct.value();

	// 아이템 확인
	CItemManager itemManager;
	auto opt_item = itemManager.getItem(product.itemID);
	if (!opt_item.has_value())
	{
		user->disconnect();
		return false;
	}
	st_Item item = opt_item.value().setQuantity(buyQuantity);

	// 구매 조건 체크
	if(product.moneyID != 0)
	{
		// 재화 구매
		DWORD userMoney = user->inventory.getMoney(product.moneyID);
		if (userMoney < product.moneyPrice * buyQuantity)
		{
			// 구매 재화 부족
			CPacket packet;
			Packet_Init_Buy_Store_Item(packet, static_cast<BYTE>(en_PACKET_STATUS_BUY_STORE_ITEM::LACK_MONEY));
			user->sendMassage(packet);
			return false;
		}

		if (!user->inventory.isAddItem(item))
		{
			// 인벤토리 부족
			CPacket packet;
			Packet_Init_Buy_Store_Item(packet, static_cast<BYTE>(en_PACKET_STATUS_BUY_STORE_ITEM::FALL_INVENTORY));
			user->sendMassage(packet);
			return false;
		}


		// 재화, 아이템 메모리 갱신
		DWORD finalMoney = user->inventory.subMoney(product.moneyID, product.moneyPrice * buyQuantity);
		auto optInventoryUpdateInfo = user->inventory.addItem(item);
		
		// 재화, 아이템 DB 갱신
		vector<string> querys;
		querys.push_back(Query_Update_User_Money(user->Nickname, product.moneyID, finalMoney));
		optInventoryUpdateInfo.value().getQuery(user->Nickname, querys);
		user->getField()->DB_Write.RequestTransactionQuery(querys, nullptr);

		// user 전송
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
		// Token 구매
		st_Item token = itemManager.getItem(product.tokenID).value().setQuantity(product.tokenPrice);
		DWORD tokenQuantity = user->inventory.getItemQuantity(token);
		if (tokenQuantity < product.tokenPrice)
		{
			// 구매 토큰 부족
			CPacket packet;
			Packet_Init_Buy_Store_Item(packet, static_cast<BYTE>(en_PACKET_STATUS_BUY_STORE_ITEM::LACK_TOKEN));
			user->sendMassage(packet);
			return false;
		}

		if (!user->inventory.isAddItem(item))
		{
			// 인벤토리 부족
			CPacket packet;
			Packet_Init_Buy_Store_Item(packet, static_cast<BYTE>(en_PACKET_STATUS_BUY_STORE_ITEM::FALL_INVENTORY));
			user->sendMassage(packet);
			return false;
		}
		
		// 토큰, 아이템 메모리 갱신
		auto tokenSlotUpdateInfo = user->inventory.removeItem(token).value();
		auto itemSlotUpdateInfo = user->inventory.addItem(item).value();
			
		// 토큰, 아이템 DB 갱신
		vector<string> querys;
		tokenSlotUpdateInfo.getQuery(user->Nickname, querys);
		itemSlotUpdateInfo.getQuery(user->Nickname, querys);
		user->getField()->DB_Write.RequestTransactionQuery(querys, nullptr);

		// user 전송
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
		LOG(L"server_StoreMananger", CLog::LEVEL_ERROR, L"아이템 데이터 시트 오류");
		return false;
	}
}


bool CStoreManager::sell(const DWORD& itemID, const DWORD& quantity, st_User* user)
{
	// 미구현
	// 구현 방식 
	// 1. item 정보 테이블( + st_Item)에 itemID에 따른 판매 가격 설정
	// 2. itemID를 가지고 와서 반영합니다.
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
		// { 상점ID, 아이템ID, 재화ID, 재화 가격, 토큰ID, 토큰 재화 가격 }
		CCsvParser CsvParser("상점 리스트.csv");
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