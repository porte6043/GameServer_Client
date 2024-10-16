#include "CCsvParser.h"


CCsvParser::CCsvParser(const string& filename) : filename(filename)
{

}

CCsvParser ::~CCsvParser()
{

}


vector<vector<string>> CCsvParser::Parse()
{
    int col = 0;
    vector<vector<string>> data;
    std::ifstream file(filename);

    if (!file.is_open()) 
    {
        std::cerr << "Error opening file: " << filename << std::endl;
        return data;
    }

    string line;
    while (getline(file, line))
    {
        vector<string> row = ParseLine(line);
        if (col == 0)
            col = row.size();
        if(row.size() == col)
            data.push_back(row);
    }

    file.close();
    return data;
}


vector<string> CCsvParser::ParseLine(const string& line)
{
    vector<string> tokens;
    std::stringstream ss(line);
    string token;

    while (getline(ss, token, ',')) 
    {
        tokens.push_back(token);
    }

    return tokens;
}