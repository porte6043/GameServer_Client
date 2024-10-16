#ifndef __CCSVPASER__
#define __CCSVPASER__
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
using std::vector;
using std::string;

class CCsvParser {
private:
    string filename;

public:
    CCsvParser(const string& filename);
    ~CCsvParser();

    // CSV ������ �а� �� ���� ������ ���ͷ� ��ȯ
    vector<vector<string>> Parse();

private:
    // CSV �� ���� �Ľ��Ͽ� ���ͷ� ��ȯ
    vector<string> ParseLine(const string& line);
};

#endif


