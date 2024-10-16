#include "DB.h"

#include "���� ���̺귯��/Log.h"
#include "CriticalSectionWrapping.h"



DB::DB(int threadcount)
{
	InitializeCriticalSection(&CS_RequestQueue);
	InitializeCriticalSection(&CS_RespondQueue);

	/*
	* mysql �ʱ�ȭ
	* mysql_init ���� Ư�� ������ �� mysql_library_init�� ȣ���ϴµ� mysql_library_init�� thread safe �����ʱ� ������ mysql_init�� thread safe���� �ʴ�
	* ���� mysql_init�� ��Ƽ���� ����� ��� mutex ���� ����ְų� �̸� mysql_library_init�� ȣ���� �ָ� thread safe �ϴ�
	*/
	if (mysql_library_init(0, NULL, NULL))
	{
		LOG(L"server_DB", CLog::LEVEL_ERROR, L"mysql_library_init �ʱ�ȭ ����");


		// �� �ڵ�� null �����͸� �������Ͽ� ũ���ø� �߻���ŵ�ϴ�.
		int* Nint = NULL;
		*Nint = 0;
	}

	MysqlPool.TlsPoolInit(1, threadcount);

	isDone = false;
	hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);	// Auto Reset
	hThreads = new HANDLE[threadcount];
	for (int iCnt = 0; iCnt < threadcount; ++iCnt)
	{
		hThreads[iCnt] = (HANDLE)_beginthreadex(nullptr, 0, DBThreadWrapping, this, 0, nullptr);
		if (hThreads[iCnt] == 0)
		{
			LOG(L"server_DB", CLog::LEVEL_ERROR, L"DB ������ ���� ����");

			// �� �ڵ�� null �����͸� �������Ͽ� ũ���ø� �߻���ŵ�ϴ�.
			int* Nint = NULL;
			*Nint = 0;
		}
	}
}

DB::~DB()
{
	isDone = true;
	SetEvent(hEvent);

	// ��� ������ ���� ���
	WaitForMultipleObjects(sizeof(hThreads) / sizeof(HANDLE), hThreads, TRUE, INFINITE);
	for (int iCnt = 0; iCnt < 4; ++iCnt)
	{
		CloseHandle(hThreads[iCnt]);
	}
	CloseHandle(hEvent);

	DeleteCriticalSection(&CS_RequestQueue);
	DeleteCriticalSection(&CS_RespondQueue);
}

void DB::RequestQuery(const string& query, const callback_function& function)
{
	lock_cs lock(&CS_RequestQueue);
	RequestQueue.push({ {query}, function });
	SetEvent(hEvent);
	return;
}
void DB::RequestTransactionQuery(const vector<string>& querys, const callback_function& function)
{
	lock_cs lock(&CS_RequestQueue);
	RequestQueue.push({ querys, function });
	SetEvent(hEvent);
	return;
}
void DB::RequestLastQuery(const callback_function& Function)
{
	lock_cs lock(&CS_RequestQueue);
	RequestQueue.push({ {}, Function });
	SetEvent(hEvent);
}
void DB::RespondQuery()
{
	while(1)
	{
		lock_cs lock(&CS_RespondQueue);
		if (RespondQueue.empty())
			break;

		respond_query respond = RespondQueue.front();
		RespondQueue.pop();

		// function ó��
		respond.funcion(respond.sql_res, respond.errorno);
	}

	return;
}


unsigned WINAPI DB::DBThreadWrapping(LPVOID p)
{
	DB* This = reinterpret_cast<DB*>(p);
	This->DBThread();
	return 0;
}
void DB::DBThread()
{
	mysql* Mysql = MysqlPool.Alloc();
	while (!isDone)
	{
		// ������ ���
		WaitForSingleObject(hEvent, INFINITE);

		if (RequestQueue.empty())
			continue;

		while (1)
		{
			// Dequeue
			EnterCriticalSection(&CS_RequestQueue);
			if (RequestQueue.empty())
			{
				LeaveCriticalSection(&CS_RequestQueue);
				break;
			}

			auto& Requests = RequestQueue.front();
			RequestQueue.pop();
			LeaveCriticalSection(&CS_RequestQueue);

			vector<string>& Querys = Requests.querylist;
			callback_function func = Requests.funcion;


			//mysql_query == 0 ����
			//mysql_errno == 0 ���� ����

			// Query ó��
			MYSQL_RES* sql_result = nullptr;
			unsigned int errorno = 0;
			switch (Querys.size())
			{
			case 0:
			{
				sql_result = nullptr;
				errorno = 0;
			}break;

			case 1:
			{
				// ���� ���� ����
				if (mysql_query(&Mysql->conn, Querys[0].c_str()))
				{
					// mysql_query ����
					sql_result = nullptr;
					errorno = mysql_errno(&Mysql->conn);
				}
				else
				{
					// mysql_query ����
					sql_result = mysql_store_result(&Mysql->conn);
					errorno = 0;	
				}
			}break;

			default:
			{
				// Ʈ������ ���� ����
				try
				{
					if (mysql_query(&Mysql->conn, "START TRANSACTION"))
						throw 0;

					for (auto& Query : Querys)
					{
						if (mysql_query(&Mysql->conn, Query.c_str()))
							throw 1;
					}

					if (mysql_query(&Mysql->conn, "COMMIT"))
						throw 1;
				}
				catch (int e)
				{
					errorno = mysql_errno(&Mysql->conn);
					LOG(L"DB", CLog::LEVEL_ERROR, L"failed transaction mysql_query [error:%s]", mysql_error(&Mysql->conn));
					if (e == 1)
						mysql_query(&Mysql->conn, "ROLLBACK");
				}
			}break;
			}


			// Enqueue
			{
				lock_cs lock(&CS_RespondQueue);
				RespondQueue.push({ sql_result, errorno , func });
			}

		}
	}
	MysqlPool.Free(Mysql);

	SetEvent(hEvent);
	return;
}