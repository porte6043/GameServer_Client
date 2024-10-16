#include "ServerStatusType.h"

string getServerStatus(BYTE status)
{
	switch (static_cast<en_SERVER_STATUS_TYPE>(status))
	{
	case en_SERVER_STATUS_TYPE::GREEN:
		return "Green";
	case en_SERVER_STATUS_TYPE::ORANGE:
		return "Orange";
	case en_SERVER_STATUS_TYPE::RED:
		return "Red";
	}
}