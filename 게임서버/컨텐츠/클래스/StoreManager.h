#ifndef __LOBBYSERVER_STOREMANAGER_H__
#define __LOBBYSERVER_STOREMANAGER_H__

#include <Windows.h>
#include <optional>
#include <map>
#include <vector>
#include <string>
using std::map;
using std::vector;
using std::string;
using std::optional;

class st_User;


struct st_Product
{
	DWORD	itemID;
	BYTE	moneyID;
	DWORD	moneyPrice;
	BYTE	tokenID;
	DWORD	tokenPrice;

	st_Product() {}
	st_Product(const string& ItemID, const string& MoneyID, const string& MoneyPrice, const string& TokenID, const string& TokenPrice)
	{
		this->itemID = std::stoul(ItemID);
		this->moneyID = std::stoi(MoneyID);
		this->moneyPrice = std::stoul(MoneyPrice);
		this->tokenID = std::stoi(TokenID);
		this->tokenPrice = std::stoul(TokenPrice);
	}
};

class CStoreManager
{
public:
	vector<st_Product> getStoreList(const BYTE& StoreCode);

	bool setStoreList();

	bool buy(const BYTE& storeID, const DWORD& itemID, const DWORD& buyQuantity, st_User* user);

	bool sell(const DWORD& itemID, const DWORD& quantity, st_User* user);

private:
	optional<st_Product> getStoreProduct(const BYTE& storeID, const DWORD& ItemID);
};


#endif