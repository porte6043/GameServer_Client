#include "Query.h"



string Query_Select_Account(const string& id)
{
	const char* format = R"(
	SELECT
		id, password, uid
	FROM
		moonserver_accountdb.account
	WHERE
		id = '%s';
    )";

	char query[1024];
	sprintf_s(query, sizeof(query), format, id.c_str());

	return query;
}
