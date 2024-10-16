#include "Money.h"

string getMoneyName(const BYTE& moneyId)
{
	switch (static_cast<en_TYPE_MONEY>(moneyId))
	{
	case en_TYPE_MONEY::DUMMY:
		return "";
	case en_TYPE_MONEY::MESO:
		return "메소";
	case en_TYPE_MONEY::CASH:
		return "캐쉬";
	}
}