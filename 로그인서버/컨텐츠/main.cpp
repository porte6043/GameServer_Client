#include "LoginServer.h"


int main(void)
{
	LoginServer Server;
	Server.Start(Server.serverSet);
	cout << "로그인 서버" << endl;

	while (1)
	{

	}

	return 0;
}