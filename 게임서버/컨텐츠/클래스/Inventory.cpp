#include "Inventory.h"

#include "공용 라이브러리/Log.h"
#include "ItemManager.h"

// 전역 함수
#include "Query.h"

// 구조체
#include "Money.h"

// enum
#include "en_LobbyServer.h"



void st_InventoryUpdateInfo::pushUpdateInfo(const st_SlotUpdateInfo& slotUpdateInfo)
{
	slotsUpdateInfo.push_back(slotUpdateInfo);
	return;
}

void st_InventoryUpdateInfo::getQuery(const string& nickname, vector<string>& querys)
{
	for (auto& slotUpdateInfo : slotsUpdateInfo)
	{
		switch (slotUpdateInfo.type)
		{
		case en_TYPE_INVENTORY_UPDATE::INSERT_INVENTORY:
			querys.push_back(Query_Insert_User_Item(nickname, slotUpdateInfo.itemID, slotUpdateInfo.quantity, slotUpdateInfo.seq));
			break;
		case en_TYPE_INVENTORY_UPDATE::UPDATE_INVENTORY:
			querys.push_back(Query_Update_User_Item(nickname, slotUpdateInfo.itemID, slotUpdateInfo.quantity, slotUpdateInfo.seq));
			break;
		case en_TYPE_INVENTORY_UPDATE::DELETE_INVENTORY:
			querys.push_back(Query_Delete_User_Item(nickname, slotUpdateInfo.seq));
			break;
		default:
			LOG(L"server_inventory", CLog::LEVEL_ERROR, L"create query type error");
		}
	}
}







CInventory::CInventory() : inventories(5)
{

}

void CInventory::clear()
{
	moneys.clear();
	for (int idx = 1;  idx < inventories.size(); ++idx)
	{
		inventories[idx].clear();
	}
}




DWORD CInventory::getMoney(const BYTE& moneyID)
{
	return moneys.at(moneyID);
}

DWORD CInventory::addMoney(const BYTE& moneyID, const DWORD& quantity)
{
	DWORD& addQuantity = moneys.at(moneyID);
	addQuantity += quantity;
	return addQuantity;
}

DWORD CInventory::subMoney(const BYTE& moneyID, const DWORD& quantity)
{
	DWORD& subQuantity = moneys.at(moneyID);
	subQuantity -= quantity;
	return subQuantity;
}

vector<st_Money> CInventory::getMoneyList() const
{
	vector<st_Money> moneyList;

	for (auto& money : moneys)
	{
		BYTE	moneyID = money.first;
		DWORD	quantity = money.second;
		moneyList.push_back({ moneyID, quantity });
	}

	return moneyList;
}

void CInventory::setMoney(const BYTE& moneyID, const DWORD& quantity)
{
	moneys.insert({ moneyID, quantity });
}




optional<st_InventoryUpdateInfo> CInventory::addItem(const st_Item& item)
{
	st_InventoryUpdateInfo inventoryUpdateInfo;


	DWORD itemTotalCount = getItemQuantity(item);
	if (itemTotalCount % item.maxQuantity == 0)
	{
		DWORD maxAddCount = inventories[item.type].usableSlots * item.maxQuantity;
		if (maxAddCount >= item.quantity)
		{
			// 새로운 슬롯에 추가
			DWORD addCount = item.quantity;
			while (1)
			{
				if (addCount >= item.maxQuantity)
				{
					auto slotUpdateInfo = insertSlot(item, item.maxQuantity);
					inventoryUpdateInfo.pushUpdateInfo(slotUpdateInfo);
					addCount -= item.maxQuantity;
				}
				else
				{
					auto slotUpdateInfo = insertSlot(item, addCount);
					inventoryUpdateInfo.pushUpdateInfo(slotUpdateInfo);
					return inventoryUpdateInfo;
				}
			}
		}
		else
			return std::nullopt;
	}
	else
	{
		DWORD maxAddCount = inventories[item.type].usableSlots * item.maxQuantity + item.maxQuantity - (itemTotalCount % item.maxQuantity);
		if (maxAddCount >= item.quantity)
		{
			DWORD addCount = item.quantity;

			// 기존 존재하는 슬롯에 업데이트
			for (auto& slot : inventories[item.type].slots[item.itemID])
			{
				auto& slotSeq = slot.first;
				auto& slotQuantity = slot.second;
				
				if (slotQuantity == item.maxQuantity)
					continue;

				if (item.maxQuantity >= slotQuantity + addCount)
				{
					// 기존 슬롯 업데이트
					slotQuantity += addCount;
					inventoryUpdateInfo.pushUpdateInfo({ en_TYPE_INVENTORY_UPDATE::UPDATE_INVENTORY, item.itemID, slotSeq, slotQuantity });
					return inventoryUpdateInfo;
				}
				else
				{
					// 기존 슬롯 업데이트 + 새로운 슬롯에 추가
					// 기존 슬롯 업데이트
					slotQuantity = item.maxQuantity;
					inventoryUpdateInfo.pushUpdateInfo({ en_TYPE_INVENTORY_UPDATE::UPDATE_INVENTORY, item.itemID, slotSeq, slotQuantity });
					addCount -= item.maxQuantity - (itemTotalCount % item.maxQuantity);

					// 새로운 슬롯에 추가
					while (1)
					{
						if (addCount >= item.maxQuantity)
						{
							addCount -= item.maxQuantity;
							auto slotUpdateInfo = insertSlot(item, item.maxQuantity);
							inventoryUpdateInfo.pushUpdateInfo(slotUpdateInfo);
						}
						else
						{
							auto slotUpdateInfo = insertSlot(item, addCount);
							inventoryUpdateInfo.pushUpdateInfo(slotUpdateInfo);
							return inventoryUpdateInfo;
						}
					}
				}
			}
		}
		else
			return std::nullopt;
	}
}

optional<st_InventoryUpdateInfo> CInventory::removeItem(const st_Item& item)
{
	st_InventoryUpdateInfo inventoryUpdateInfo;

	DWORD totalItemCount = getItemQuantity(item);
	if (totalItemCount >= item.quantity)
	{
		auto& inventory = inventories[item.type];
		auto& slot = inventories[item.type].slots[item.itemID];

		DWORD slotDeleteCount = -1;
		DWORD removeCount = item.quantity;
		for (auto rIter = slot.rbegin(); rIter != slot.rend(); ++rIter)
		{
			DWORD slotSeq = rIter->first;
			DWORD& slotQuantity = rIter->second;

			if (removeCount > slotQuantity)
			{
				removeCount -= slotQuantity;
				++slotDeleteCount;
				inventoryUpdateInfo.pushUpdateInfo( deleteSlot(item, slotSeq) );
			}
			else if (removeCount == slotQuantity)
			{
				removeCount -= slotQuantity;
				++slotDeleteCount;
				inventoryUpdateInfo.pushUpdateInfo( deleteSlot(item, slotSeq) );
				break;
			}
			else
			{
				slotQuantity -= removeCount;
				inventoryUpdateInfo.pushUpdateInfo({ en_TYPE_INVENTORY_UPDATE::UPDATE_INVENTORY, item.itemID, slotSeq, slotQuantity });
				break;
			}
		}
		
		if(slotDeleteCount != -1)
		{
			auto rBeingIter = slot.end();
			auto rEndIter = slot.rbegin(); advance(rEndIter, slotDeleteCount);
			slot.erase(next(rEndIter).base(), rBeingIter);
		}

		if (slot.size() == 0)
			inventory.slots.erase(item.itemID);

		return inventoryUpdateInfo;
	}
	else
		return std::nullopt;
}

DWORD CInventory::getItemQuantity(const st_Item& item)
{
	DWORD totalQuantity = 0;

	if (!containItem(item))
		return totalQuantity;

	auto& itemSlots = inventories[item.type].slots[item.itemID];
	for (auto& iter : itemSlots)
	{
		totalQuantity += iter.second;
	}

	return totalQuantity;
}

bool CInventory::isAddItem(const st_Item& item)
{
	DWORD maxAddCount = inventories[item.type].usableSlots * item.maxQuantity + item.maxQuantity - (getItemQuantity(item) % item.maxQuantity);
	return maxAddCount >= item.quantity;

}

vector<st_ItemInfo> CInventory::getItemList() const
{
	vector<st_ItemInfo> itemList;

	for (auto& inventory : inventories)
	{
		for (auto& slot : inventory.slots)
		{
			DWORD itemID = slot.first;
			for (auto& slotData : slot.second)
			{
				DWORD seq = slotData.first;
				DWORD quantity = slotData.second;
				itemList.push_back({ itemID, seq, quantity });
			}
		}
	}

	return itemList;
}

void CInventory::setItem(const st_Item& item, const DWORD& quantity, const DWORD& seq)
{
	auto& inventory = inventories[item.type];
	inventory.slots[item.itemID][seq] = quantity;
	inventory.availableSlot[seq] = false;
	--inventory.usableSlots;
	if (inventory.nextSlotSeq == seq)
	{
		for (int idx = inventory.nextSlotSeq + 1; idx < inventory.availableSlot.size(); ++idx)
		{
			if (inventory.availableSlot[idx])
			{
				inventory.nextSlotSeq = idx;
				break;
			}
		}
	}
	else
		inventory.nextSlotSeq = min(inventory.nextSlotSeq, seq);
}






DWORD CInventory::getNextSlotSeq(const st_Item& Item)
{
	auto& inventory = inventories[Item.type];

	DWORD nextSlotSeq = inventory.nextSlotSeq;
	for (int seq = inventory.nextSlotSeq + 1; seq < inventory.availableSlot.size(); ++seq)
	{
		if (inventory.availableSlot[seq])
		{
			inventory.nextSlotSeq = seq;
			break;
		}
	}
	--inventory.usableSlots;
	inventory.availableSlot[nextSlotSeq] = false;

	return nextSlotSeq;
}

bool CInventory::existSlot(const st_Item& item)
{
	return inventories[item.type].usableSlots;
}

bool CInventory::containItem(const st_Item& item)
{
	return inventories[item.type].slots.find(item.itemID) != inventories[item.type].slots.end();
}

st_SlotUpdateInfo CInventory::insertSlot(const st_Item& item, const DWORD& quantity)
{
	auto& inventory = inventories[item.type];

	DWORD newSlotSeq = getNextSlotSeq(item);
	inventory.slots[item.itemID][newSlotSeq] = quantity;

	return { en_TYPE_INVENTORY_UPDATE::INSERT_INVENTORY, item.itemID, newSlotSeq, quantity };
}

st_SlotUpdateInfo CInventory::deleteSlot(const st_Item& item, const DWORD& seq)
{
	auto& inventory = inventories[item.type];

	inventory.availableSlot[seq] = true;
	++inventory.usableSlots;
	inventory.nextSlotSeq = min(inventory.nextSlotSeq, seq);
	return { en_TYPE_INVENTORY_UPDATE::DELETE_INVENTORY, item.itemID, seq, 0 };
}