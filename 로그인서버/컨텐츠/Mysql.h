#ifndef __LOBBYSERVER_MYSQL_H__
#define __LOBBYSERVER_MYSQL_H__
#include <iostream>
#include <WinSock2.h>

#include "mysql/include/mysql.h"
#include "mysql/include/errmsg.h"
#pragma comment(lib, "mysql/lib/libmysql.lib")

struct mysql
{
	MYSQL conn;	// mysql connection
	mysql();
	mysql(mysql& other) = delete;
	mysql(const mysql& other) = delete;
	mysql(const mysql&& other) = delete;
	mysql& operator=(const mysql& other) = delete;
	~mysql();
};
#endif // __LOBBYSERVER_DB_H__
