#ifndef __LOBBYSERVER_DB_H__
#define __LOBBYSERVER_DB_H__

#include <windows.h>
#include <string>
#include <queue>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <functional>
#include <thread>
using std::vector;
using std::string;
using std::queue;
using std::unordered_set;
using std::unordered_map;
using std::function;


#include "공용 라이브러리/CTlsPool.h"
#include "Mysql.h"


class DB
{
	using callback_function = std::function<void(MYSQL_RES*, unsigned int)>;

	struct request_query
	{
		std::vector<std::string> querylist;
		callback_function funcion;
	};

	struct respond_query
	{
		MYSQL_RES* sql_res;
		unsigned int errorno;
		callback_function funcion;
	};

private:
	bool						isDone;				// 스레드 종료 플래그
	HANDLE*						hThreads;			// Accept ID
	HANDLE						hEvent;				// WaitForSingleObject 용도
	CTlsPool<mysql>				MysqlPool;

	std::queue<request_query>		RequestQueue;
	CRITICAL_SECTION			CS_RequestQueue;

	std::queue<respond_query>		RespondQueue;
	CRITICAL_SECTION			CS_RespondQueue;


public:		DB() = delete;
public:		DB(int threadcount);
public:		~DB();

public:	
	void RequestQuery(const std::string& query, const callback_function& function);

	void RequestTransactionQuery(const std::vector<std::string>& querys, const callback_function& function);
	
	void RequestLastQuery(const callback_function& function);

	void RespondQuery();

private:	static unsigned WINAPI DBThreadWrapping(LPVOID p);
private:	void DBThread();

};
#endif