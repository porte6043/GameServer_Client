#ifndef __TEXTPARSER__
#define __TEXTPARSER__

#include <string>
using std::string;
using std::wstring;

class CTextPasing;
class CTextPasingCategory;

class CTextPasingCategory
{
	char* Buffer;
	unsigned int BufferSize;

public: CTextPasingCategory();
public: CTextPasingCategory(char* buffer, int bufferSize);

private: void SkipNone(int& index);

public: bool GetValueChar(char* pValue, const char* findword);
public: bool GetValueChar(string& str, const char* findword);
public: bool GetValueWChar(wchar_t* pValue, const char* findword);
public: bool GetValueWChar(wstring& pValue, const char* findword);

public: bool GetValueInt(int* pValue, const char* findword);
public: bool GetValueShort(short* shValue, const char* findword);
public: bool GetValueByte(char* pValue, const char* findword);
public: bool GetValueBool(bool* pValue, const char* findword);



};

class CTextPasing
{
private:
	char* Buffer;
	unsigned int BufferSize;
	CTextPasingCategory* Category[64];
	int CategoryIndex;
	
public: CTextPasing();
public: ~CTextPasing();

public: CTextPasingCategory* FindCategory(const char* findcategory);
private: void SkipNone(int& index);

public: bool GetLoadData(const char* filename);

public: bool GetValueChar(char* pValue, const char* findword);
public: bool GetValueChar(string& str, const char* findword);
public: bool GetValueWChar(wchar_t* pValue, const char* findword);
public: bool GetValueWChar(wstring& pValue, const char* findword);

public: bool GetValueInt(int* pValue, const char* findword);
public: bool GetValueShort(short* shValue, const char* findword);
public: bool GetValueByte(char* pValue, const char* findword);
public: bool GetValueBool(bool* pValue, const char* findword);



};















int GetFileSize(const char* filename);

void GetLoadData(char* pbuffer, int size, const char* filename);

bool GetValueChar(char* pbuffer, char* pvalue, const char* findword, int filesize = 0);

bool GetValueInt(char* pbuffer, int* pvalue, const char* findword, int filesize);

void SkinNone(char* pbuffer);

#endif