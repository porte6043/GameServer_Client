#include "Mysql.h"
#include "���� ���̺귯��/Log.h"

mysql::mysql()
{
	mysql_init(&conn);
	MYSQL* connection = mysql_real_connect(&conn, "127.0.0.1", "root", "qwer1234!!", "moonserver_accountdb", 3306, (char*)NULL, 0);
	if (connection == nullptr)
	{
		LOG(L"server_Mysql", CLog::LEVEL_ERROR, L"mysql ���� ����");

		// �� �ڵ�� null �����͸� �������Ͽ� ũ���ø� �߻���ŵ�ϴ�.
		int* Nint = NULL;
		*Nint = 0;
	}
}

mysql::~mysql()
{
	mysql_close(&conn);
}

