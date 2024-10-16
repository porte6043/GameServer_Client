#ifndef __LOBBYSERVER_ITEMMANAGER_H__
#define __LOBBYSERVER_ITEMMANAGER_H__
#include <Windows.h>
#include <memory>
#include <string>
#include <optional>
#include <functional>  // std::reference_wrapper 포함
using std::string;
using std::optional;
using std::reference_wrapper;


struct st_Item
{
	BYTE	category;		// 아이템 카테고리 ex) 	
	BYTE	type;		// 아이템 종류 ex) 일반, 소비, 장비
	string	name;
	DWORD	itemID;
	string	description;
	DWORD	maxQuantity;
	DWORD	quantity;
	DWORD	seq;

	st_Item(const string& category, const string& type, const string& name, const string& itemID, const string& description, const string& maxQuantity) :
		name(name),
		description(description)
	{
		this->category = (BYTE)std::stoi(category);
		this->type = std::stoi(type);
		this->itemID = std::stoul(itemID);
		this->maxQuantity = std::stoul(maxQuantity);
		this->quantity = 1;
		this->seq = 0;
	}

	st_Item& setQuantity(const DWORD& quantity)
	{
		this->quantity = quantity;
		return *this;
	}
};


class st_User;

class CItemManager
{
public:
	optional<st_Item> getItem(const DWORD& itemID);
	optional<reference_wrapper<const st_Item>> getItemByRef(const DWORD& itemID);

	bool setItemList();

	bool useItem(const DWORD& itemID, st_User* user);

private:
	bool useNomalItem(const DWORD& itemID, st_User* user);
	bool usePackageItem(const DWORD& itemID, st_User* user);
	bool useCashItem(const DWORD& itemID, st_User* user);


	// nomal item
	bool execute_StrBox(const DWORD& itemID, st_User* user);
	bool execute_DexBox(const DWORD& itemID, st_User* user);
	bool execute_IntBox(const DWORD& itemID, st_User* user);
	bool execute_LukBox(const DWORD& itemID, st_User* user);

	// packege item
	bool execute_AllBox(const DWORD& itemID, st_User* user);
	bool execute_StrBoxAndCash(const DWORD& itemID, st_User* user);

	// cash item
	bool execute_Preminum_StrBox(const DWORD& itemID, st_User* user);
	bool execute_Preminum_DexBox(const DWORD& itemID, st_User* user);
	bool execute_Preminum_IntBox(const DWORD& itemID, st_User* user);
	bool execute_Preminum_LukBox(const DWORD& itemID, st_User* user);
		  


};


#endif