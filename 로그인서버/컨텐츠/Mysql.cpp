#include "Mysql.h"
#include "공용 라이브러리/Log.h"

mysql::mysql()
{
	mysql_init(&conn);
	MYSQL* connection = mysql_real_connect(&conn, "127.0.0.1", "root", "qwer1234!!", "moonserver_accountdb", 3306, (char*)NULL, 0);
	if (connection == nullptr)
	{
		LOG(L"server_Mysql", CLog::LEVEL_ERROR, L"mysql 연결 실패");

		// 이 코드는 null 포인터를 역참조하여 크래시를 발생시킵니다.
		int* Nint = NULL;
		*Nint = 0;
	}
}

mysql::~mysql()
{
	mysql_close(&conn);
}

