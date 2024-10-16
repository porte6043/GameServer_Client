#ifndef __LOBBYSERVER_MYSQL_H__
#define __LOBBYSERVER_MYSQL_H__
#include <iostream>
#include <WinSock2.h>

#include "mysql/include/mysql.h"
#include "mysql/include/errmsg.h"
#pragma comment(lib, "mysql/lib/libmysql.lib")

#include "공용 라이브러리/Log.h"

struct mysql
{
	MYSQL conn;	// mysql connection
	mysql()
	{
		mysql_init(&conn);
		MYSQL* connection = mysql_real_connect(&conn, "127.0.0.1", "root", "qwer1234!!", "world_korea", 3306, (char*)NULL, 0);
		if (connection == nullptr)
		{
			LOG(L"server_Mysql", CLog::LEVEL_ERROR, L"DB 연결 실패");
			Sleep(3000);

			// 이 코드는 null 포인터를 역참조하여 크래시를 발생시킵니다.
			int* Nint = NULL;
			*Nint = 0;
		}
	}
	~mysql()
	{
		mysql_close(&conn);
	}
};
#endif // __LOBBYSERVER_DB_H__
