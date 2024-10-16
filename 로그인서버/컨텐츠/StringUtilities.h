#ifndef  __LOBBYSERVER_STRINGUTILITIES_H__
#define __LOBBYSERVER_STRINGUTILITIES_H__
#include <string>
#include <vector>
#include <sstream>
using std::string;
using std::vector;


string CombineStrings(std::initializer_list<string> values, const char combiner);

// ���ڿ��� ������ ����� ��ȯ�Ѵ�
vector<string> SplitString(const std::string& str, const char delimiter);



#endif //  __LOBBYSERVER_STRINGUTILITIES_H__
