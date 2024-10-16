#ifndef __LOBBYSERVER_CITEMMANAGER_H__
#define __LOBBYSERVER_CITEMMANAGER_H__
#include <Windows.h>
#include <map>
#include <vector>
#include <stack>
#include <optional>
#include <string>
using std::string;
using std::map;
using std::vector;
using std::stack;
using std::optional;


enum class en_TYPE_INVENTORY_UPDATE;
struct st_Money;
struct st_Item;

struct st_inventory
{
	map<DWORD, map<DWORD, DWORD>>	slots;			// { ItemID, { Seq, Quantity } }
	vector<bool>					availableSlot;	// seq 사용 여부
	DWORD							nextSlotSeq;	// 비어 있는 slotSeq
	DWORD							usableSlots;	// 사용 가능한 slot 수
	DWORD							maxSlots;		// 최대 slot 수

	st_inventory() :
		availableSlot(129, true),
		nextSlotSeq(1),
		usableSlots(128),
		maxSlots(128) {}

	void clear()
	{
		slots.clear();
		std::fill(availableSlot.begin(), availableSlot.end(), true);
		nextSlotSeq = 1;
		usableSlots = 128;
	}
};

struct st_SlotUpdateInfo
{
	en_TYPE_INVENTORY_UPDATE	type;
	DWORD						itemID;
	DWORD						seq;
	DWORD						quantity;
};

struct st_InventoryUpdateInfo
{
	vector<st_SlotUpdateInfo>	slotsUpdateInfo;

	void pushUpdateInfo(const st_SlotUpdateInfo& slotUpdateInfo);

	void getQuery(const string& nickname, vector<string>& querys);
};

struct st_ItemInfo
{
	DWORD	itemID;
	DWORD	seq;
	DWORD	quantity;
};



class CInventory
{
	map<BYTE, DWORD>		moneys;			// { moneyID, quantity }

	vector<st_inventory>	inventories;

// 공용
public:
	CInventory();

	void clear();

// 메소
public:
	DWORD getMoney(const BYTE& moneyID);

	DWORD addMoney(const BYTE& moneyID, const DWORD& quantity);

	DWORD subMoney(const BYTE& moneyID, const DWORD& quantity);

	vector<st_Money> getMoneyList() const;

	void setMoney(const BYTE& moneyID, const DWORD& quantity);


// 아이템
public:
	optional<st_InventoryUpdateInfo> addItem(const st_Item& item);

	optional<st_InventoryUpdateInfo> removeItem(const st_Item& item);

	DWORD getItemQuantity(const st_Item& item);

	bool isAddItem(const st_Item& item);

	vector<st_ItemInfo> getItemList() const;

	void setItem(const st_Item& item, const DWORD& quantity, const DWORD& seq);

private:
	DWORD getNextSlotSeq(const st_Item& Item);

	bool existSlot(const st_Item& item);

	bool containItem(const st_Item& item);

	st_SlotUpdateInfo insertSlot(const st_Item& item, const DWORD& quantity);

	st_SlotUpdateInfo deleteSlot(const st_Item& item, const DWORD& seq);

	


};

#endif