#include "StringUtilities.h"

// combiner���� ������� ���ڿ��� ��ģ��
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

// ���ڿ��� ������ ����� ��ȯ�Ѵ�
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