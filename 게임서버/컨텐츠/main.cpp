#include <conio.h>
#include "LobbyServer.h"


int main()
{
	CLobbyServer Server;
	Server.Start(Server.ServerSet);
	std::cout << "게임 서버" << std::endl;



	while (1)
	{
		Sleep(1000);

		// 키보드
		if (_kbhit())
		{
			// 눌린 값 대입
			switch (_getch())
			{
			case 'e':
				Server.End();
				std::cout << "게임 서버 종료" << std::endl;
				break;
			default:
				break;
			}
		}
	}

	return 0;
}