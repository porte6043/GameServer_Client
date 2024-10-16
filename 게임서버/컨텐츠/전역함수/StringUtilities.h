#ifndef  __LOBBYSERVER_STRINGUTILITIES_H__
#define __LOBBYSERVER_STRINGUTILITIES_H__
#include <string>
#include <vector>
#include <sstream>
using std::string;
using std::vector;


string CombineStrings(std::initializer_list<string> values, const char combiner);

// 문자열을 단위로 나누어서 반환한다
vector<string> SplitString(const std::string& str, const char delimiter);



#endif //  __LOBBYSERVER_STRINGUTILITIES_H__
