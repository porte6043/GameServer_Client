#include <conio.h>
#include "LobbyServer.h"


int main()
{
	CLobbyServer Server;
	Server.Start(Server.ServerSet);
	std::cout << "���� ����" << std::endl;



	while (1)
	{
		Sleep(1000);

		// Ű����
		if (_kbhit())
		{
			// ���� �� ����
			switch (_getch())
			{
			case 'e':
				Server.End();
				std::cout << "���� ���� ����" << std::endl;
				break;
			default:
				break;
			}
		}
	}

	return 0;
}