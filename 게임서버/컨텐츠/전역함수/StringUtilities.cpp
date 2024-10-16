#include "StringUtilities.h"

// combiner으로 구분지어서 문자열을 합친다
string CombineStrings(std::initializer_list<string> values, const char combiner)
{
	string result;
	for (auto iter = values.begin(); iter != values.end(); ++iter) {
		if (iter != values.begin()) {
			result += combiner;
		}
		result += *iter;
	}

	return result;
}

// 문자열을 단위로 나누어서 반환한다
vector<string> SplitString(const std::string& str, const char delimiter)
{
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream stream(str);

	while (std::getline(stream, token, delimiter)) {
		tokens.push_back(token);
	}

	return tokens;
}