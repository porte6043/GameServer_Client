#pragma once
#include <Windows.h>
#include <vector>
#include <string>
using std::vector;
using std::string;

struct st_CharacterInfo
{
	string	nickname;
	BYTE	seq;
};

struct st_ChannelInfo
{
	string	name;
	string	ip;
	WORD	port;
	BYTE	status; //en_SERVER_STATUS_TYPE
};

struct st_Product
{
	BYTE	type;
	DWORD	id;
	string	name;
	string	description;

	BYTE	moneyID;
	DWORD	moneyPrice;
	BYTE	tokenID;
	DWORD	tokenPrice;
};

struct st_Friend
{
	string	name;
	bool	isLogin;
};

struct st_AcceptAddFriendStatus
{
	bool	status;
	string	nickname;
};

struct st_RecvWhisper
{
	
};

struct st_Data
{
	vector<st_CharacterInfo>	characterList;
	string						createCharacterNickname;
	bool						createCharacterAvailableNicknameStatus;
	bool						createCharacterStatus;

	vector<st_ChannelInfo>		channelInfo;
	vector<st_Product>			productInfo;
	string						buyItemStatus;
	string						useItemStatusInfo;
	vector<st_Friend>			friendInfo;
	string						whishperNickname;
	bool						whishperStatus;
	st_AcceptAddFriendStatus	acceptAddFriendStatus;
	bool						addFriendStatus;
	string						_whishperNickname;
	bool						_whishperStatus;
	bool						_isWhisper;
};
