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

    // CSV 파일을 읽고 각 행을 벡터의 벡터로 반환
    vector<vector<string>> Parse();

private:
    // CSV 한 줄을 파싱하여 벡터로 반환
    vector<string> ParseLine(const string& line);
};

#endif


