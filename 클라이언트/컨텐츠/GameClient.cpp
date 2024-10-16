#include "GameClient.h"
#include "LoginClient.h"
#include "LobbyProtocol.h"
#include "PacketInit.h"
#include "ServerStatusType.h"
#include "GlobalWindowForm.h"
using namespace ns_GameClient;



GameClient::GameClient() : isMoveChannel(false)
{
	// ���� ����
	CTextPasing config;
	config.GetLoadData("Server.ini");
	CTextPasingCategory* ChatServer = config.FindCategory("Client");
	ChatServer->GetValueWChar(ip, "IP");
	ChatServer->GetValueShort((short*)&port, "Port");
	ChatServer->GetValueByte((char*)&packetCode, "PacketCode");
	ChatServer->GetValueByte((char*)&packetKey, "PacketKey");

	InitializeSRWLock(&SRW_logicQueue);
	InitializeSRWLock(&SRW_cmdQueue);
}

// ������ ������� ��
void GameClient::OnConnectServer()
{
	if (isMoveChannel)
	{
		directLogin();
		isMoveChannel = false;
	}
	else
		Login();


	return;
}

// ������ ������ �������� ��
void GameClient::OnDisconnectServer()
{
	if(isMoveChannel)
	{
		while (!ReConnect(gameServerInfo.ip, gameServerInfo.port)) {}
	}
	else
		cout << endl << "������ ������ ���������ϴ�..." << endl;
}

// Message ���� �Ϸ� �� Contents Packet ó���� �մϴ�.
void GameClient::OnRecv(CPacket& packet)
{
	AcquireSRWLockExclusive(&SRW_logicQueue);
	logicQueue.push(packet);
	ReleaseSRWLockExclusive(&SRW_logicQueue);
	return;
}




// Lobby packet Res
void GameClient::Packet_Login(CPacket& packet)
{
	DWORD64 characterCount;
	packet >> characterCount;
	data.characterList.resize(characterCount + 1);
	for (int idx = 1; idx <= characterCount; ++idx)
	{
		st_CharacterInfo& characterInfo = data.characterList[idx];
		packet.GetData(characterInfo.nickname, 20);
		packet >> characterInfo.seq;
	}

	setScene([this](string cmd) { Scene_Login(cmd); });
	return;
}
void GameClient::Packet_Login_Other_Channel(CPacket& packet)
{
	setScene([this](string cmd) { Scene_Field(cmd); });
	return;
}
void GameClient::Packet_Select_Character(CPacket& packet)
{
	packet.GetData(character.nickname, 20);
	packet >> character.uid >> character.roomNo;

	DWORD64 statsCount;
	packet >> statsCount;
	for (int icnt = 0; icnt < statsCount; ++icnt)
	{
		st_Stats stats;
		packet >> stats.id >> stats.stats;
		packet.GetData(stats.name, 20);
		character.stats.insert({ stats.id, stats });
	}

	DWORD64 moneyCount;
	packet >> moneyCount;
	for (int icnt = 0; icnt < moneyCount; ++icnt)
	{
		st_Money money;
		packet >> money.id >> money.quantity;
		packet.GetData(money.name, 20);
		character.moneys.insert({ money.id, money });
	}

	DWORD maxSlot;
	DWORD64 itemCount;
	packet >> maxSlot >> itemCount;
	for (int icnt = 1; icnt <= 5; ++icnt)
	{
		// ���� �� Ȯ��
		character.items[icnt].resize(maxSlot + 1);
	}
	for (int icnt = 0; icnt < itemCount; ++icnt)
	{
		st_Item item;
		packet >> item.type >> item.itemID;
		packet.GetData(item.name, 64);
		packet.GetData(item.description, 512);
		packet >> item.maxQuantity >> item.seq >> item.quantity;

		character.items[item.type][item.seq] = item;
	}

	DWORD64 friendCount;
	packet >> friendCount;
	character.friends.resize(friendCount + 1);
	for (int idx = 1; idx <= friendCount; ++idx)
	{
		packet.GetData(character.friends[idx], 20);
	}

	DWORD64 addFriendCount;
	packet >> addFriendCount;
	character.addFriends.resize(addFriendCount + 1);
	for (int idx = 1; idx <= addFriendCount; ++idx)
	{
		packet.GetData(character.addFriends[idx], 20);
	}

	return;
}
void GameClient::Packet_CharacterList(CPacket& packet)
{
	DWORD64 characterCount;
	packet >> characterCount;
	data.characterList.resize(characterCount + 1);
	for (int idx = 1; idx <= characterCount; ++idx)
	{
		st_CharacterInfo& characterInfo = data.characterList[idx];
		packet.GetData(characterInfo.nickname, 20);
		packet >> characterInfo.seq;
	}

	setScene([this](string cmd) { Scene_Login(cmd); });
	return;
}
void GameClient::Packet_Available_Nickname(CPacket& packet)
{
	packet >> data.createCharacterAvailableNicknameStatus;
	
	setScene([this](string cmd) { Scene_CreateCharacter_Available_NicknameStatus(cmd); });
	return;
}
void GameClient::Packet_Create_Character(CPacket& packet)
{
	packet >> data.createCharacterStatus;

	setScene([this](string cmd) { Scene_CreateCharacterStatus(cmd); });
	return;
}
// Field packet Res
void GameClient::Packet_Move_Field(CPacket& packet)
{
	BYTE status;
	DWORD roomNo;
	string fieldName;
	packet >> status >> roomNo;
	packet.GetData(fieldName, 20);

	if (status)
	{
		// �ʵ� �̵� ����
		// ... �ʵ� �̵� ������ �غ� �մϴ�
		if (roomNo == static_cast<DWORD>(en_ROOM_NUMBER::Lobby))
		{
			// �κ�� �̵�
			character.clear();
			character.roomNo = roomNo;
			character.fieldName = std::move(fieldName);
			setScene([this](string cmd) { Scene_Login(cmd); });
			return;
		}

		character.roomNo = roomNo;
		character.fieldName = std::move(fieldName);
		setScene([this](string cmd) { Scene_Field(cmd); });

		CPacket packet;
		Packet_Init_Ready_Field(packet);
		SendMSG(packet);
	}
	else
	{
		// �ʵ� �̵� ����
		// �ƹ��͵� ���ϰ� ���� �ʵ忡�� ��� �ֽ��ϴ�.
	}

	return;
}
void GameClient::Packet_Move_Channel(CPacket& packet)
{
	packet.GetData(gameServerInfo.token, 64);
	isMoveChannel = true;
	Disconnect();
	setScene([this](string cmd) { Scene_MoveChannel(cmd); });

	return;
}
void GameClient::Packet_Channel_List(CPacket& packet)
{
	DWORD64 channelCount;
	packet >> channelCount;
	data.channelInfo.resize(channelCount + 1);
	for (int idx = 1; idx <= channelCount; ++idx)
	{
		packet.GetData(data.channelInfo[idx].name, 20);
		packet.GetData(data.channelInfo[idx].ip, 16);
		packet >> data.channelInfo[idx].port;
		packet >> data.channelInfo[idx].status;
	}

	setScene([this](string cmd) { Scene_ChannelList(cmd); });
	return;
}
void GameClient::Packet_Store_Product_List(CPacket& packet)
{
	DWORD64 productCount;
	packet >> productCount;
	data.productInfo.resize(productCount + 1);
	for (int idx = 1; idx <= productCount; ++idx)
	{
		packet >> data.productInfo[idx].type >> data.productInfo[idx].id;
		packet.GetData(data.productInfo[idx].name, 64);
		packet.GetData(data.productInfo[idx].description, 512);
		packet >> data.productInfo[idx].moneyID >> data.productInfo[idx].moneyPrice;
		packet >> data.productInfo[idx].tokenID >> data.productInfo[idx].tokenPrice;
	}

	setScene([this](string cmd) { Scene_StoreProductList(cmd); });
	return;
}
void GameClient::Packet_Buy_Store_Item(CPacket& packet)
{
	BYTE status;
	packet >> status;
	if (static_cast<en_PACKET_STATUS_BUY_STORE_ITEM>(status) != en_PACKET_STATUS_BUY_STORE_ITEM::SUCCESS)
	{
		data.buyItemStatus = "������ ���ſ� �����߽��ϴ�.";
		setScene([this](string cmd) { Scene_BuyItemStatus(cmd); });
		return;
	}

	st_Money money;
	packet >> money.id >> money.quantity;
	character.moneys[money.id].quantity = money.quantity;


	BYTE tokenID;
	DWORD64 tokenSlotUpdateCount;
	packet >> tokenID >> tokenSlotUpdateCount;
	for (int icnt = 1; icnt <= tokenSlotUpdateCount; ++icnt)
	{
		DWORD seq;
		DWORD quantity;
		packet >> seq >> quantity;

		// token�� �Ϲ� ������
		character.items[1][seq].quantity = quantity;
	}

	st_Item item;
	DWORD64	itemSlotUpdateCount;
	packet >> item.type >> item.itemID;
	packet.GetData(item.name, 64);
	packet.GetData(item.description, 512);
	packet >> item.maxQuantity >> itemSlotUpdateCount;
	for (int icnt = 1; icnt <= itemSlotUpdateCount; ++icnt)
	{
		packet >> item.seq >> item.quantity;
		character.items[item.type][item.seq] = item;
	}

	data.buyItemStatus = "������ ���ſ� �����߽��ϴ�.";
	setScene([this](string cmd) { Scene_BuyItemStatus(cmd); });
	return;
}
void GameClient::Packet_Show_Me_The_Money(CPacket& packet)
{
	BYTE	moneyID;
	DWORD	moneyQuantity;
	packet >> moneyID >> moneyQuantity;
	character.moneys[moneyID].quantity = moneyQuantity;
	setScene([this](string cmd) { Scene_ShowMeTheMoney(cmd); });
}
void GameClient::Packet_Use_Item_Failed(CPacket& packet)
{
	BYTE status;
	packet >> status;
	// status�� ���� �������� ��� ���� ���� ����

	data.useItemStatusInfo = "������ ��뿡 ���� �߽��ϴ�.";
	setScene([this](string cmd) { Scene_UseItem(cmd); });
	return;
}
void GameClient::Packet_Use_Item_Stats_Box(CPacket& packet)
{
	BYTE	statsID;
	DWORD	stats;
	packet >> statsID >> stats;

	st_Item item;
	DWORD64	itemSlotUpdateCount;
	packet >> item.type >> item.itemID;
	packet.GetData(item.name, 64);
	packet.GetData(item.description, 512);
	packet >> item.maxQuantity >> itemSlotUpdateCount;
	for (int icnt = 1; icnt <= itemSlotUpdateCount; ++icnt)
	{
		packet >> item.seq >> item.quantity;
		character.items[item.type][item.seq] = item;
	}


	character.stats[statsID].stats = stats;
	data.useItemStatusInfo = character.stats[statsID].name + "box ������� ���� " + character.stats[statsID].name + "�� �����߽��ϴ�.";
	setScene([this](string cmd) { Scene_UseItem(cmd); });
	return;
}
void GameClient::Packet_Add_Friend(CPacket& packet)
{
	packet >> data.addFriendStatus;
	setScene([this](string cmd) { Scene_AddFriendStatus(cmd); });
}
void GameClient::Packet_Req_Friend(CPacket& packet)
{
	string nickname;
	packet.GetData(nickname, 20);

	// ģ�� ��û �߰� 
	character.addFriends.push_back(std::move(nickname));
}
void GameClient::Packet_Accept_Friend(CPacket& packet)
{
	string nickname;
	packet.GetData(nickname, 20);

	// ģ�� �߰� 
	character.friends.push_back(nickname);
}
void GameClient::Packet_Accept_Add_Friend(CPacket& packet)
{
	packet >> data.acceptAddFriendStatus.status;
	packet.GetData(data.acceptAddFriendStatus.nickname, 20);

	if (data.acceptAddFriendStatus.status)
	{
		auto iter = std::find_if(character.addFriends.begin(), character.addFriends.end(),
			[nick = data.acceptAddFriendStatus.nickname](string nickname) {
				return nickname == nick;
			});
		character.addFriends.erase(iter);
	}

	setScene([this](string cmd) { Scene_AcceptAddFriendStatus(cmd); });
	return;
}
void GameClient::Packet_Friend_List(CPacket& packet)
{
	DWORD64 friendCount;
	packet >> friendCount;
	data.friendInfo.resize(friendCount + 1);
	for (int idx = 1; idx <= friendCount; ++idx)
	{
		packet.GetData(data.friendInfo[idx].name, 20);
		packet.GetData(&data.friendInfo[idx].isLogin, 1);
	}

	setScene([this](string cmd) { Scene_FriendList(cmd); });
	return;
}
void GameClient::Packet_Send_Whisper(CPacket& packet)
{
	BYTE status;
	packet >> status;
	data.whishperStatus = status;
	setScene([this](string cmd) { Scene_WhishperStatus(cmd); });
	return;
}
void GameClient::Packet_Req_Send_Whishper(CPacket& packet)
{
	string sendNickname;
	string chatting;
	packet.GetData(sendNickname, 20);
	packet.GetData(chatting, 512);

	cout << endl << "[�ӼӸ� ����][" << sendNickname << "] : " << chatting << endl;
	cout << currentCmdPath;
}


// server connect
void GameClient::login()
{
	LoginClient loginClient;
	while (!loginClient.Connect(ip.c_str(), port, 1, 1, true, true, false, packetCode, packetKey)) {}
	gameServerInfo = loginClient.getGameServerInfo();

	while (!Connect(gameServerInfo.ip.c_str(), gameServerInfo.port, 1, 1, true, true, false, packetCode, packetKey)) {}

	return;
}

void GameClient::directLogin()
{
	CPacket packet;
	Packet_Init_Login_Other_Channel(packet, gameServerInfo.uid, gameServerInfo.token, character.nickname);
	SendMSG(packet);
}

// Req To Server
void GameClient::Login()
{
	// ��ū üũ
	CPacket packet;
	Packet_Init_Login(packet, gameServerInfo.uid, gameServerInfo.token);
	SendMSG(packet);
}
void GameClient::AvailableNickname(string nickname)
{
	CPacket packet;
	Packet_Init_Available_Nickname(packet, nickname);
	SendMSG(packet);
}
void GameClient::CreateCharacter(string nickname)
{
	CPacket packet;
	Packet_Init_Create_Character(packet, nickname);
	SendMSG(packet);
}
void GameClient::CharacterList()
{
	CPacket packet;
	Packet_Init_CharacterList(packet);
	SendMSG(packet);
}
void GameClient::Select_Character(string nickname)
{
	CPacket packet;
	Packet_Init_Select_Character(packet, nickname);
	SendMSG(packet);
}
void GameClient::MoveField(DWORD roomNo)
{
	CPacket packet;
	Packet_Init_Move_Field(packet, roomNo);
	SendMSG(packet);
	return;
}
void GameClient::MoveChennel(string serverName, string ip, WORD port)
{
	gameServerInfo.serverName = serverName;
	gameServerInfo.ip.assign(ip.begin(), ip.end());
	gameServerInfo.port = port;

	CPacket packet;
	Packet_Init_Move_Channel(packet, ip, port);
	SendMSG(packet);
	return;
}
void GameClient::ChannelList()
{
	CPacket packet;
	Packet_Init_Channel_List(packet);
	SendMSG(packet);
	return;
}
void GameClient::StoreProductList(BYTE storeID)
{
	CPacket packet;
	Packet_Init_Store_Product_List(packet, storeID);
	SendMSG(packet);
	return;
}
void GameClient::BuyStoreItem(BYTE storeID, DWORD itemID, DWORD quantity)
{
	CPacket packet;
	Packet_Init_Buy_Store_Item(packet, storeID, itemID, quantity);
	SendMSG(packet);
	return;
}
void GameClient::ShowMeTheMoney()
{
	CPacket packet;
	Packet_Init_Show_Me_The_Money(packet, 1);
	SendMSG(packet);
	return;
}
void GameClient::UseItem(DWORD itemID)
{
	CPacket packet;
	Packet_Init_Use_Item(packet, itemID);
	SendMSG(packet);
	return;
}
void GameClient::AddFriend(string nickname)
{
	CPacket packet;
	Packet_Init_Add_Friend(packet, nickname);
	SendMSG(packet);
	return;
}
void GameClient::AcceptAddFriend(string nickname)
{
	CPacket packet;
	Packet_Init_Accept_Add_Friend(packet, nickname);
	SendMSG(packet);
}
void GameClient::FriendList()
{
	CPacket packet;
	Packet_Init_Friend_List(packet);
	SendMSG(packet);
	return;
}
void GameClient::SendWhishper(string recvNickname, string chatting)
{
	CPacket packet;
	Packet_Init_Send_Whishper(packet, recvNickname, chatting);
	SendMSG(packet);
}



void GameClient::update()
{
	while (!logicQueue.empty())
	{
		AcquireSRWLockExclusive(&SRW_logicQueue);
		CPacket packet = logicQueue.front();
		logicQueue.pop();
		ReleaseSRWLockExclusive(&SRW_logicQueue);


		int type;
		packet >> type;
		switch (type)
		{
		case en_PACKET_SC_LOBBY_RES_LOGIN:
			Packet_Login(packet);
			break;
		case en_PACKET_CS_LOBBY_REQ_LOGIN_OTHER_CHANNEL:
			Packet_Login(packet);
			break;
		case en_PACKET_SC_LOBBY_RES_SELECT_CHARACTER:
			Packet_Select_Character(packet);
			break;
		case en_PACKET_SC_LOBBY_RES_CHARACTERLIST:
			Packet_CharacterList(packet);
			break;
		case en_PACKET_SC_LOBBY_RES_AVAILABLE_NICKNAME:
			Packet_Available_Nickname(packet);
			break;
		case en_PACKET_SC_LOBBY_RES_CREATE_CHARACTER:
			Packet_Create_Character(packet);
			break;
		case en_PACKET_SC_LOBBY_RES_MOVE_FIELD:
			Packet_Move_Field(packet);
			break;
		case en_PACKET_SC_LOBBY_RES_MOVE_CHANNEL:
			Packet_Move_Channel(packet);
			break;
		case en_PACKET_SC_LOBBY_RES_CHANNEL_LIST:
			Packet_Channel_List(packet);
			break;
		case en_PACKET_SC_LOBBY_RES_STORE_PRODUCT_LIST:
			Packet_Store_Product_List(packet);
			break;
		case en_PACKET_SC_LOBBY_RES_BUY_STORE_ITEM:
			Packet_Buy_Store_Item(packet);
			break;
		case en_PACKET_SC_LOBBY_RES_SHOW_ME_THE_MONEY:
			Packet_Show_Me_The_Money(packet);
			break;
		case en_PACKET_SC_LOBBY_RES_USE_ITEM_FAILED:
			Packet_Use_Item_Failed(packet);
			break;
		case en_PACKET_SC_LOBBY_RES_USE_ITEM_STATS_BOX:
			Packet_Use_Item_Stats_Box(packet);
			break;
		case en_PACKET_SC_LOBBY_RES_ADD_FRIEND:
			Packet_Add_Friend(packet);
			break;
		case en_PACKET_SC_LOBBY_REQ_REQ_FRIEND:
			Packet_Req_Friend(packet);
			break;
		case en_PACKET_SC_LOBBY_REQ_ACCEPT_FRIEND:
			Packet_Accept_Friend(packet);
			break;
		case en_PACKET_SC_LOBBY_RES_ACCEPT_ADD_FRIEND:
			Packet_Accept_Add_Friend(packet);
			break;
		case en_PACKET_SC_LOBBY_RES_FRIEND_LIST:
			Packet_Friend_List(packet);
			break;
		case en_PACKET_SC_LOBBY_RES_SEND_WHISPER:
			Packet_Send_Whisper(packet);
			break;
		case en_PACKET_SC_LOBBY_REQ_SEND_WHISPER:
			Packet_Req_Send_Whishper(packet);
			break;

		}
	}
	return;
}

void GameClient::cmd()
{
	while (!cmdQueue.empty())
	{
		AcquireSRWLockExclusive(&SRW_cmdQueue);
		auto pair = cmdQueue.front();
		cmdQueue.pop();
		ReleaseSRWLockExclusive(&SRW_cmdQueue);

		if (currentScene.target<void(*)(string)>() == pair.first.target<void(*)(string)>())
		{
			// ������ Scene�� ���
			pair.first(pair.second);
		}
	}
	return;
}



// scene
void GameClient::setScene(function<void(string)> scene)
{
	currentScene = scene;
	currentScene("");
	return;
}
void GameClient::setCmd(string cmd)
{
	AcquireSRWLockExclusive(&SRW_cmdQueue);
	cmdQueue.push({ currentScene , cmd });
	ReleaseSRWLockExclusive(&SRW_cmdQueue);
	return;
}

void GameClient::Scene_Login(string cmd)
{
	if (cmd.empty())
	{
		// ó������ �ش� ������ ����� ���
		currentCmdPath = "[�κ�â] > ";
		cout << currentCmdPath;

		GlobalWindowForm::cmdForm->updateLabelCommand(L"��ɾ� ����(��ȣ�� �Է�)\r\n\r\n1. ĳ���� ����\r\n\r\n2. ĳ���� ���");
		return;
	}
	else
	{
		// ��ɾ� ���
		cout << cmd << endl;
	}

	int cmdNo;
	try
	{
		cmdNo = stoi(cmd);
	}
	catch (std::exception e)
	{
		cout << "�߸��� ��ɾ� �Դϴ�." << endl;
		cout << currentCmdPath;
		return;
	}

	switch (cmdNo)
	{
	case 1:
		setScene([this](string cmd) { Scene_CreateCharacter(cmd); });
		break;
	case 2:
		setScene([this](string cmd) { Scene_SelectCharacter(cmd); });
		break;
	default:
		cout << "�߸��� ��ɾ� �Դϴ�." << endl;
		cout << currentCmdPath;
		break;
	}

	return;
}
void GameClient::Scene_CreateCharacter(string cmd)
{
	if (cmd.empty())
	{
		// ó������ �ش� ������ ����� ���
		currentCmdPath = "[�κ�â]\\[ĳ���� ����]\\ > ";
		cout << "������ ĳ���� �̸��� �Է��ϼ���(20��)" << endl;
		cout << currentCmdPath;

		GlobalWindowForm::cmdForm->updateLabelCommand(L"��ɾ� ����(��ȣ�� �Է�)\r\n\r\n0. �ڷ� ����");
		return;
	}
	else
	{
		// ��ɾ� ���
		cout << cmd << endl;
	}

	try
	{
		int cmdNo = stoi(cmd);
		if (cmdNo == 0)
		{
			setScene([this](string cmd) { Scene_Login(cmd); });
			return;
		}
	}
	catch (std::exception e) {}

	if (cmd.size() > 20)
	{
		cout << "ĳ���� �̸��� 20�� �̸��Դϴ�." << endl;
		cout << currentCmdPath;
		return;
	}

	data.createCharacterNickname = cmd;
	AvailableNickname(cmd);
	return;
}
void GameClient::Scene_CreateCharacterStatus(string cmd)
{
	if (!data.createCharacterStatus)
	{
		// ���� ����
		cout << "��� �Ұ����� �г��� �Դϴ�." << endl;
		setScene([this](string cmd) { Scene_CreateCharacter(cmd); });
		return;
	}

	cout << "ĳ���� ������ �����߽��ϴ�." << endl;
	CharacterList();
	return;
}
void GameClient::Scene_CreateCharacter_Available_NicknameStatus(string cmd)
{
	if (!data.createCharacterAvailableNicknameStatus)
	{
		cout << "��� �Ұ����� �г��� �Դϴ�." << endl;
		setScene([this](string cmd) { Scene_CreateCharacter(cmd); });
		return;
	}

	cout << "ĳ���� ���� ��...." << endl;

	CreateCharacter(data.createCharacterNickname);
	return;
}
void GameClient::Scene_SelectCharacter(string cmd)
{
	if (cmd.empty())
	{
		// ó������ �ش� ������ ����� ���
		cout << "ĳ���� ���" << endl;
		std::sort(data.characterList.begin(), data.characterList.end(), [](const st_CharacterInfo& a, const st_CharacterInfo& b) {
			return a.seq < b.seq; });
		for (int idx = 1; idx < data.characterList.size(); ++idx)
		{
			auto& charInfo = data.characterList[idx];
			cout << idx << ". " << charInfo.nickname << endl;
		}
		currentCmdPath = "[�κ�â]\\[ĳ���� ���]\\ > ";
		cout << currentCmdPath;


		GlobalWindowForm::cmdForm->updateLabelCommand(L"��ɾ� ����(��ȣ�� �Է�)\r\n\r\n0. �ڷΰ���");
		return;
	}
	else
	{
		// ��ɾ� ���
		cout << cmd << endl;
	}


	int cmdNo;
	try
	{
		cmdNo = stoi(cmd);
	}
	catch (std::exception e)
	{
		cout << "�߸��� ��ɾ� �Դϴ�." << endl;
		cout << currentCmdPath;
		return;
	}

	if (cmdNo == 0)
	{
		// �ڷ� ���� ��ɾ�
		setScene([this](string cmd) { Scene_Login(cmd); });
		return;
	}

	if (cmdNo < 0 || cmdNo >= data.characterList.size())
	{
		// �������� �ʴ� �ʵ� �̵� ��û
		cout << "�߸��� ��ɾ� �Դϴ�." << endl;
		cout << currentCmdPath;
		return;
	}

	Select_Character(data.characterList[cmdNo].nickname);
	return;
}

void GameClient::Scene_Field(string cmd)
{
	if (cmd.empty())
	{
		// ó������ �ش� ������ ����� ���
		currentCmdPath = "[" + character.nickname + "][ä��:" + gameServerInfo.serverName + "]:\\" + "[�ʵ�:" + character.fieldName + "]\\ > ";
		cout << currentCmdPath;
		
		GlobalWindowForm::cmdForm->updateLabelCommand(L"��ɾ� ����(��ȣ�� �Է�)\r\n\r\n1. �ʵ� �̵�\r\n\r\n2. ä�� �̵�\r\n\r\n3. ���� ������ ����\r\n\r\n4. �κ��丮\r\n\r\n5. Show me the mone"
			L"y\r\n\r\n6. ģ��â\r\n\r\n7. �ӼӸ�\r\n\r\n8. ����\r\n\r\n9. �κ�â �̵�");
		return;
	}
	else
	{
		// ��ɾ� ���
		cout << cmd << endl;
	}

	int cmdNo;
	try
	{
		cmdNo = stoi(cmd);
	}
	catch (std::exception e)
	{
		cout << "�߸��� ��ɾ� �Դϴ�." << endl;
		cout << currentCmdPath;
		return;
	}

	switch (cmdNo)
	{
	case 1:
		setScene([this](string cmd){ Scene_MoveField(cmd); });
		break;
	case 2:
		ChannelList();
		break;
	case 3:
		StoreProductList(1);
		break;
	case 4:
		setScene([this](string cmd) { Scene_inventory(cmd); });
		break;
	case 5:
		ShowMeTheMoney();
		break;
	case 6:
		setScene([this](string cmd) { Scene_Friend(cmd); });
		break;
	case 7:
		setScene([this](string cmd) { Scene_Whisper(cmd); });
		break;
	case 8:
		setScene([this](string cmd) { Scene_CharacterStats(cmd); });
		break;
	case 9:
		MoveField(0);
		break;
	default:
		cout << "�߸��� ��ɾ� �Դϴ�." << endl;
		cout << currentCmdPath;
		break;
	}
}
void GameClient::Scene_MoveField(string cmd)
{
	if (cmd.empty())
	{
		// ó������ �ش� ������ ����� ���
		cout << "�̵� ������ �ʵ� ����Ʈ" << endl;
		RoomManager	roomManager;
		auto& RoomMap = roomManager.getRoomMap();
		for (auto iter : RoomMap)
		{
			cout << static_cast<DWORD>(iter.first) << ". " << iter.second << endl;
		}
		currentCmdPath = "[" + character.nickname + "][ä��:" + gameServerInfo.serverName + "]:\\" + "[�ʵ�:" + character.fieldName + "]\\[�ʵ� ����Ʈ]\\ > ";
		cout << currentCmdPath;


		GlobalWindowForm::cmdForm->updateLabelCommand(L"��ɾ� ����(��ȣ�� �Է�)\r\n\r\n0. �ڷΰ���");
		return;
	}
	else
	{
		// ��ɾ� ���
		cout << cmd << endl;
	}


	int cmdNo;
	try
	{
		cmdNo = stoi(cmd);
	}
	catch (std::exception e)
	{
		cout << "�߸��� ��ɾ� �Դϴ�." << endl;
		cout << currentCmdPath;
		return;
	}

	if (cmdNo == 0)
	{
		// �ڷ� ���� ��ɾ�
		setScene([this](string cmd) { Scene_Field(cmd); });
		return;
	}

	RoomManager	roomManager;
	if (roomManager.getRoomName(cmdNo).empty())
	{
		// �������� �ʴ� �ʵ� �̵� ��û
		cout << "�߸��� ��ɾ� �Դϴ�." << endl;
		cout << currentCmdPath;
		return;
	}

	MoveField(cmdNo);
	return;
}
void GameClient::Scene_MoveChannel(string cmd)
{
	if(cmd.empty())
		cout << "ä�� �̵� ��..." << endl;
}
void GameClient::Scene_ChannelList(string cmd)
{
	if (cmd.empty())
	{
		// ó������ �ش� ������ ����� ���
		cout << "�̵� ������ ä�� ����Ʈ" << endl;
		for (int idx = 1; idx < data.channelInfo.size(); ++idx)
		{
			cout << idx << ". " << data.channelInfo[idx].name << ":" << getServerStatus(data.channelInfo[idx].status) << endl;
		}
		currentCmdPath = "[" + character.nickname + "][ä��:" + gameServerInfo.serverName + "]:\\" + "[�ʵ�:" + character.fieldName + "]\\[ä�� ����Ʈ]\\ > ";
		cout << currentCmdPath;


		GlobalWindowForm::cmdForm->updateLabelCommand(L"��ɾ� ����(��ȣ�� �Է�)\r\n\r\n0. �ڷΰ���");
		return;
	}
	else
	{
		// ��ɾ� ���
		cout << cmd << endl;
	}


	int cmdNo;
	try
	{
		cmdNo = stoi(cmd);
	}
	catch (std::exception e)
	{
		cout << "�߸��� ��ɾ� �Դϴ�." << endl;
		cout << currentCmdPath;
		return;
	}

	if (cmdNo == 0)
	{
		// �ڷ� ���� ��ɾ�
		setScene([this](string cmd) { Scene_Field(cmd); });
		return;
	}

	if (cmdNo < 0 || cmdNo >= data.channelInfo.size())
	{
		// �������� �ʴ� �ʵ� �̵� ��û
		cout << "�߸��� ��ɾ� �Դϴ�." << endl;
		cout << currentCmdPath;
		return;
	}

	MoveChennel(data.channelInfo[cmdNo].name, data.channelInfo[cmdNo].ip, data.channelInfo[cmdNo].port);
	return;
}
void GameClient::Scene_StoreProductList(string cmd)
{
	if (cmd.empty())
	{
		// ó������ �ش� ������ ����� ���
		cout << "[������ �̸�] [�޼� ����] ���� ������ ����Ʈ:������ ��ǰ ��ȣ�� �Է��Ͻÿ�" << endl;
		for (int idx = 1; idx < data.productInfo.size(); ++idx)
		{
			cout << idx << ". " << data.productInfo[idx].name << "\t" << data.productInfo[idx].moneyPrice << "�޼�" << endl;
		}
		currentCmdPath = "[" + character.nickname + "][ä��:" + gameServerInfo.serverName + "]:\\" + "[�ʵ�:" + character.fieldName + "]\\[����]\\ > ";
		cout << currentCmdPath;


		GlobalWindowForm::cmdForm->updateLabelCommand(L"��ɾ� ����(��ȣ�� �Է�)\r\n\r\n0. �ڷΰ���");
		return;
	}
	else
	{
		// ��ɾ� ���
		cout << cmd << endl;
	}


	int cmdNo;
	try
	{
		cmdNo = stoi(cmd);
	}
	catch (std::exception e)
	{
		cout << "�߸��� ��ɾ� �Դϴ�." << endl;
		cout << currentCmdPath;
		return;
	}

	if (cmdNo == 0)
	{
		// �ڷ� ���� ��ɾ�
		setScene([this](string cmd) { Scene_Field(cmd); });
		return;
	}

	if (cmdNo < 0 || cmdNo >= data.productInfo.size())
	{
		// �������� �ʴ� �ʵ� �̵� ��û
		cout << "�߸��� ��ɾ� �Դϴ�." << endl;
		cout << currentCmdPath;
		return;
	}

	BuyStoreItem(1, data.productInfo[cmdNo].id, 1);
	return;
}
void GameClient::Scene_BuyItemStatus(string cmd)
{
	if (cmd.empty())
	{
		cout << data.buyItemStatus << endl;
		setScene([this](string cmd) { Scene_StoreProductList(cmd); });
	}
	return;
}
void GameClient::Scene_inventory(string cmd)
{
	if (cmd.empty())
	{
		// ó������ �ش� ������ ����� ���
		currentCmdPath = "[" + character.nickname + "][ä��:" + gameServerInfo.serverName + "]:\\" + "[�ʵ�:" + character.fieldName + "]\\[�κ��丮]\\ > ";
		cout << currentCmdPath;


		GlobalWindowForm::cmdForm->updateLabelCommand(L"��ɾ� ����(��ȣ�� �Է�)\r\n\r\n0. �ڷΰ���\r\n\r\n1. ��ȭ\r\n\r\n2. �Ϲ� ������\r\n\r\n3. �Һ� ������");
		return;
	}
	else
	{
		// ��ɾ� ���
		cout << cmd << endl;
	}

	int cmdNo;
	try
	{
		cmdNo = stoi(cmd);
	}
	catch (std::exception e)
	{
		cout << "�߸��� ��ɾ� �Դϴ�." << endl;
		cout << currentCmdPath;
		return;
	}

	if (cmdNo == 0)
	{
		// �ڷ� ���� ��ɾ�
		setScene([this](string cmd) { Scene_Field(cmd); });
		return;
	}

	switch (cmdNo)
	{
	case 1:
		setScene([this](string cmd) { Scene_inventoryMoneyList(cmd); });
		break;
	case 2:
		setScene([this](string cmd) { Scene_inventoryRegularItemList(cmd); });
		break;
	case 3:
		setScene([this](string cmd) { Scene_inventoryUsableItemList(cmd); });
		break;
	default:
		cout << "�߸��� ��ɾ� �Դϴ�." << endl;
		cout << currentCmdPath;
		break;
	}
}
void GameClient::Scene_inventoryRegularItemList(string cmd)
{
	if (cmd.empty())
	{
		// ó������ �ش� ������ ����� ���

		cout << "[������ �̸�(����)] [������ ����]" << endl;
		for (int idx = 1; idx < character.items[1].size(); ++idx)
		{
			auto& item = character.items[1][idx];
			if (item.quantity == 0)
				continue;
			cout << item.name << "(" << item.quantity << ") [" << item.description << "]" << endl;
		}
		currentCmdPath = "[" + character.nickname + "][ä��:" + gameServerInfo.serverName + "]:\\" + "[�ʵ�:" + character.fieldName + "]\\[�κ��丮]\\[�Ϲ� ������ ����Ʈ]\\ > ";
		cout << currentCmdPath;


		GlobalWindowForm::cmdForm->updateLabelCommand(L"��ɾ� ����(��ȣ�� �Է�)\r\n\r\n0. �ڷΰ���");
		return;
	}
	else
	{
		// ��ɾ� ���
		cout << cmd << endl;
	}

	int cmdNo;
	try
	{
		cmdNo = stoi(cmd);
	}
	catch (std::exception e)
	{
		cout << "�߸��� ��ɾ� �Դϴ�." << endl;
		cout << currentCmdPath;
		return;
	}

	if (cmdNo != 0)
	{
		cout << "�߸��� ��ɾ� �Դϴ�." << endl;
		cout << currentCmdPath;
		return;
	}

	// �ڷ� ���� ��ɾ�
	setScene([this](string cmd) { Scene_inventory(cmd); });
}
void GameClient::Scene_inventoryUsableItemList(string cmd)
{
	if (cmd.empty())
	{
		// ó������ �ش� ������ ����� ���

		cout << "[������ �̸�(����)] [������ ����]" << endl;
		for (int idx = 1; idx < character.items[2].size(); ++idx)
		{
			auto& item = character.items[2][idx];
			if (item.quantity == 0)
				continue;
			cout << idx << ". " << item.name << "(" << item.quantity << ") [" << item.description << "]" << endl;
		}
		currentCmdPath = "[" + character.nickname + "][ä��:" + gameServerInfo.serverName + "]:\\" + "[�ʵ�:" + character.fieldName + "]\\[�κ��丮]\\[�Һ� ������ ����Ʈ]\\ > ";
		cout << currentCmdPath;


		GlobalWindowForm::cmdForm->updateLabelCommand(L"��ɾ� ����(��ȣ�� �Է�)\r\n\r\n0. �ڷΰ���");
		return;
	}
	else
	{
		// ��ɾ� ���
		cout << cmd << endl;
	}

	int cmdNo;
	try
	{
		cmdNo = stoi(cmd);
	}
	catch (std::exception e)
	{
		cout << "�߸��� ��ɾ� �Դϴ�." << endl;
		cout << currentCmdPath;
		return;
	}

	if (cmdNo == 0)
	{
		// �ڷ� ���� ��ɾ�
		setScene([this](string cmd) { Scene_inventory(cmd); });
		return;
	}

	if (cmdNo < 0 || cmdNo >= character.items[2].size() || character.items[2][cmdNo].quantity == 0)
	{
		// �������� �ʴ� �ʵ� �̵� ��û
		cout << "�߸��� ��ɾ� �Դϴ�." << endl;
		cout << currentCmdPath;
		return;
	}

	// �Һ� ������ ���
	auto& item = character.items[2][cmdNo];
	UseItem(item.itemID);
	return;
}
void GameClient::Scene_inventoryMoneyList(string cmd)
{
	// ó������ �ش� ������ ����� ���
	cout << "[��ȭ �̸�] [������]" << endl;
	for (int moneyId = 1; moneyId <= character.moneys.size(); ++moneyId)
	{
		auto& money = character.moneys[moneyId];
		cout << money.name << "\t" << money.quantity << endl;
	}
	setScene([this](string cmd) { Scene_inventory(cmd); });
}
void GameClient::Scene_UseItem(string cmd)
{
	if (cmd.empty())
	{
		cout << data.useItemStatusInfo << endl;
		setScene([this](string cmd) { Scene_inventoryUsableItemList(cmd); });
	}
	return;
}
void GameClient::Scene_ShowMeTheMoney(string cmd)
{
	cout << "Show Me The Money" << endl;
	cout << "�޼Ұ� �þ���ϴ�." << endl;
	setScene([this](string cmd) { Scene_Field(cmd); });
	return;
}
void GameClient::Scene_Friend(string cmd)
{
	if (cmd.empty())
	{
		// ó������ �ش� ������ ����� ���
		currentCmdPath = "[" + character.nickname + "][ä��:" + gameServerInfo.serverName + "]:\\" + "[�ʵ�:" + character.fieldName + "]\\[ģ��â]\\ > ";
		cout << currentCmdPath;


		GlobalWindowForm::cmdForm->updateLabelCommand(L"��ɾ� ����(��ȣ�� �Է�)\r\n\r\n0. �ڷΰ���\r\n\r\n1. ģ�� ���\r\n\r\n2. ģ�� �߰� ���\r\n\r\n3. ģ�� �߰�");
		return;
	}
	else
	{
		// ��ɾ� ���
		cout << cmd << endl;
	}

	int cmdNo;
	try
	{
		cmdNo = stoi(cmd);
	}
	catch (std::exception e)
	{
		cout << "�߸��� ��ɾ� �Դϴ�." << endl;
		cout << currentCmdPath;
		return;
	}

	switch (cmdNo)
	{
	case 0:
		setScene([this](string cmd) { Scene_Field(cmd); });
		break;
	case 1:
		FriendList();
		break;
	case 2:
		setScene([this](string cmd) { Scene_AddFriendList(cmd); });
		break;
	case 3:
		setScene([this](string cmd) { Scene_AddFriend(cmd); });
		break;
	default:
		cout << "�߸��� ��ɾ� �Դϴ�." << endl;
		cout << currentCmdPath;
		break;
	}
}
void GameClient::Scene_FriendList(string cmd)
{
	if (cmd.empty())
	{
		// ó������ �ش� ������ ����� ���

		cout << "[ģ�� �̸�] [����]" << endl;
		for (int idx = 1; idx < data.friendInfo.size(); ++idx)
		{
			cout << idx << ". " << data.friendInfo[idx].name << "\t\t" << string(data.friendInfo[idx].isLogin ? "�¶���" : "��������") << endl;
		}
		currentCmdPath = "[" + character.nickname + "][ä��:" + gameServerInfo.serverName + "]:\\" + "[�ʵ�:" + character.fieldName + "]\\[ģ��â]\\[ģ�� ���]\\ > ";
		cout << currentCmdPath;


		GlobalWindowForm::cmdForm->updateLabelCommand(L"��ɾ� ����(��ȣ�� �Է�)\r\n\r\n0. �ڷΰ���");
		return;
	}
	else
	{
		// ��ɾ� ���
		cout << cmd << endl;
	}

	int cmdNo;
	try
	{
		cmdNo = stoi(cmd);
	}
	catch (std::exception e)
	{
		cout << "�߸��� ��ɾ� �Դϴ�." << endl;
		cout << currentCmdPath;
		return;
	}

	if (cmdNo == 0)
	{
		setScene([this](string cmd) { Scene_Friend(cmd); });
		return;
	}
	if (cmdNo < 0 || cmdNo >= data.friendInfo.size())
	{
		cout << "�߸��� ��ɾ� �Դϴ�." << endl;
		cout << currentCmdPath;
		return;
	}

	auto& Friend = data.friendInfo[cmdNo];
	data.whishperNickname = Friend.name;

	// �ӼӸ� ������ ��ȯ
	setScene([this](string cmd) { Scene_SendFriendWhishper(cmd); });
	return;
}
void GameClient::Scene_AddFriendList(string cmd)
{
	if (cmd.empty())
	{
		// ó������ �ش� ������ ����� ���
		cout << "ģ�� ���� ��û ���" << endl;
		for (int idx = 1; idx < character.addFriends.size(); ++idx)
		{
			cout << idx << ". " << character.addFriends[idx] << endl;
		}
		currentCmdPath = "[" + character.nickname + "][ä��:" + gameServerInfo.serverName + "]:\\" + "[�ʵ�:" + character.fieldName + "]\\[ģ��â][ģ�� ���� ��û ���]\\ > ";
		cout << currentCmdPath;


		GlobalWindowForm::cmdForm->updateLabelCommand(L"��ɾ� ����(��ȣ�� �Է�)\r\n\r\n0. �ڷΰ���");
		return;
	}
	else
	{
		// ��ɾ� ���
		cout << cmd << endl;
	}

	int cmdNo;
	try
	{
		cmdNo = stoi(cmd);
	}
	catch (std::exception e)
	{
		cout << "�߸��� ��ɾ� �Դϴ�." << endl;
		cout << currentCmdPath;
		return;
	}
	if (cmdNo == 0)
	{
		// �ڷ� ���� ��ɾ�
		setScene([this](string cmd) { Scene_Friend(cmd); });
		return;
	}
	if (cmdNo < 0 || cmdNo >= character.addFriends.size())
	{
		// �������� �ʴ� �ʵ� �̵� ��û
		cout << "�߸��� ��ɾ� �Դϴ�." << endl;
		cout << currentCmdPath;
		return;
	}

	AcceptAddFriend(character.addFriends[cmdNo]);
	return;
}
void GameClient::Scene_AcceptAddFriendStatus(string cmd)
{
	cout << (data.acceptAddFriendStatus.status ? "���� ����" : "���� ����") << endl;
	setScene([this](string cmd) { Scene_AddFriendList(cmd); });
	return;
}
void GameClient::Scene_AddFriend(string cmd)
{
	if (cmd.empty())
	{
		// ó������ �ش� ������ ����� ���
		currentCmdPath = "[" + character.nickname + "][ä��:" + gameServerInfo.serverName + "]:\\" + "[�ʵ�:" + character.fieldName + "]\\[ģ��â]\\[ģ�� �߰�â]\\ > ";
		cout << currentCmdPath;


		GlobalWindowForm::cmdForm->updateLabelCommand(L"��ɾ� ����(��ȣ�� �Է�)\r\n\r\n0. �ڷΰ���");
		return;
	}
	else
	{
		// ��ɾ� ���
		cout << cmd << endl;
	}

	try
	{
		int cmdNo = stoi(cmd);
		if (cmdNo == 0)
		{
			setScene([this](string cmd) { Scene_Friend(cmd); });
			return;
		}
	}
	catch (std::exception e) {}

	
	// ģ�� �߰�
	AddFriend(cmd);
	return;
}
void GameClient::Scene_AddFriendStatus(string cmd)
{
	cout << (data.addFriendStatus ? "ģ�� �߰� ��û�� �����߽��ϴ�.." : "ģ�� �߰� ��û�� �����߽��ϴ�.") << endl;
	setScene([this](string cmd) { Scene_AddFriend(cmd); });
	return;
}
void GameClient::Scene_SendFriendWhishper(string cmd)
{
	if (cmd.empty())
	{
		// ó������ �ش� ������ ����� ���
		currentCmdPath = "[" + character.nickname + "][ä��:" + gameServerInfo.serverName + "]:\\" + "[�ʵ�:" + character.fieldName + "]\\[ģ��â]\\[ģ�� ���]\\[�ӼӸ�:" + data.whishperNickname + "]\\ > ";
		cout << currentCmdPath;
		GlobalWindowForm::cmdForm->updateLabelCommand(L"��ɾ� ����(��ȣ�� �Է�)\r\n\r\n0. �ڷΰ���");
		return;
	}
	else
	{
		// ��ɾ� ���
		cout << cmd << endl;
	}

	try
	{
		int cmdNo = stoi(cmd);
		if (cmdNo == 0)
		{
			setScene([this](string cmd) { Scene_FriendList(cmd); });
			return;
		}
	}
	catch (std::exception e) {}

	// �ӼӸ� ����
	data._isWhisper = false;
	SendWhishper(data.whishperNickname, cmd);
	return;
}
void GameClient::Scene_WhishperStatus(string cmd)
{
	cout << (data.whishperStatus ? "�ӼӸ��� ���� �߽��ϴ�." : "��밡 �������� �����Դϴ�.") << endl;
	if (data._isWhisper)
		setScene([this](string cmd) { Scene_SendWhisper(cmd); });
	else
		setScene([this](string cmd) { Scene_SendFriendWhishper(cmd); });
	return;
}
void GameClient::Scene_Whisper(string cmd)
{
	if (cmd.empty())
	{
		// ó������ �ش� ������ ����� ���
		cout << "�ӼӸ��� nickname�� �Է��ϼ���." << endl;
		currentCmdPath = "[" + character.nickname + "][ä��:" + gameServerInfo.serverName + "]:\\" + "[�ʵ�:" + character.fieldName + "]\\[�ӼӸ�]\\ > ";
		cout << currentCmdPath;
		GlobalWindowForm::cmdForm->updateLabelCommand(L"��ɾ� ����(��ȣ�� �Է�)\r\n\r\n0. �ڷΰ���");
		return;
	}
	else
	{
		// ��ɾ� ���
		cout << cmd << endl;
	}

	try
	{
		int cmdNo = stoi(cmd);
		if (cmdNo == 0)
		{
			setScene([this](string cmd) { Scene_Field(cmd); });
			return;
		}
	}
	catch (std::exception e) {}

	data._whishperNickname = cmd;
	setScene([this](string cmd) { Scene_SendWhisper(cmd); });
	return;
}
void GameClient::Scene_SendWhisper(string cmd)
{
	if (cmd.empty())
	{
		// ó������ �ش� ������ ����� ���
		currentCmdPath = "[" + character.nickname + "][ä��:" + gameServerInfo.serverName + "]:\\" + "[�ʵ�:" + character.fieldName + "]\\[�ӼӸ�:" + data._whishperNickname + "]\\ > ";
		cout << currentCmdPath;
		GlobalWindowForm::cmdForm->updateLabelCommand(L"��ɾ� ����(��ȣ�� �Է�)\r\n\r\n0. �ڷΰ���");
		return;
	}
	else
	{
		// ��ɾ� ���
		cout << cmd << endl;
	}

	try
	{
		int cmdNo = stoi(cmd);
		if (cmdNo == 0)
		{
			setScene([this](string cmd) { Scene_Whisper(cmd); });
			return;
		}
	}
	catch (std::exception e) {}

	// �ӼӸ� ����
	data._isWhisper = true;
	SendWhishper(data._whishperNickname, cmd);
	return;
}
void GameClient::Scene_CharacterStats(string cmd)
{
	// ó������ �ش� ������ ����� ���
	cout << "[ĳ���� ����]" << endl;
	for (auto& iter_stats : character.stats)
	{
		auto& stats = iter_stats.second;
		cout << stats.name << " : " << stats.stats << endl;
	}
	
	setScene([this](string cmd) { Scene_Field(cmd); });
}