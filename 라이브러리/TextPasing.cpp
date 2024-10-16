#include "TextPasing.h"

//---------------------------------------------------
// rb(바이너리모드)
// Enter	: 0d 0a (\r\n)
// Space	: 20	( )
// Tap		: 09	(\t)
//---------------------------------------------------

//------------------------------------------------------------------------------------------------------------------------------
// CTextPasing
CTextPasing::CTextPasing() : Category{}
{
	Buffer = nullptr;
	BufferSize = 0;
	CategoryIndex = 0;
}
CTextPasing::~CTextPasing()
{
	if (Buffer == nullptr)
		return;

	delete[] Buffer;

	for (int iCnt = 0; iCnt < CategoryIndex; ++iCnt)
	{
		delete Category[iCnt];
	}

}

CTextPasingCategory* CTextPasing::FindCategory(const char* findcategory)
{
	char CategoryWord[256] = { 0, };
	size_t CategoryLen = strlen(findcategory);

	char* CategoryBuffer = nullptr;
	int CategorySize = 0;

	for (int iCnt = 0; iCnt < BufferSize; ++iCnt)
	{
		SkipNone(iCnt);

		// Category 찾기
		if (':' == Buffer[iCnt])
		{
			++iCnt;
			SkipNone(iCnt);
			memcpy(CategoryWord, &Buffer[iCnt], CategoryLen);
			if (0 == strcmp(findcategory, CategoryWord))
			{
				iCnt += CategoryLen;
				//---------------------------------------------------------
				// 만약에 find 단어가 E_Atk인데 
				// E_AtkShape 라는 단어가 먼저 있으면
				// E_Atk을 찾긴 하지만 (E_Atk)Shape ()부분을 찾게 된다
				// 그러므로 찾는 단어 다음에 공백문자나 /가 안오면 버그가 생긴다
				// 그래서 공백문자나 /가 아니면 찾는 단어가 아닌것이다.
				//---------------------------------------------------------
				if (Buffer[iCnt] != ' ' && Buffer[iCnt] != '\t' && Buffer[iCnt] != '\r' && Buffer[iCnt] != '/')
				{
					iCnt -= CategoryLen;
					continue;
				}
				SkipNone(iCnt);

				if (Buffer[iCnt] == '{')
				{
					++iCnt;
					SkipNone(iCnt);

					// 버퍼 위치 저장
					CategoryBuffer = &Buffer[iCnt];

					while (1)
					{
						++iCnt;
						if (Buffer[iCnt] == '}')
							break;
						++CategorySize;
					}

					CTextPasingCategory* pCategory = new CTextPasingCategory(CategoryBuffer, CategorySize);
					Category[CategoryIndex++] = pCategory;
					return pCategory;
				}
			}
		}
	}
	return nullptr;
}
void CTextPasing::SkipNone(int& index)
{
	while(Buffer[index] == ' ' || Buffer[index] == '\t' || Buffer[index] == '\r' || Buffer[index] == '\n' || Buffer[index] == '/')
	{
		// 띄어쓰기(' ') 넘기기 (Space, Tap, Enter)
		while (Buffer[index] == ' ' || Buffer[index] == '\t' || Buffer[index] == '\r' || Buffer[index] == '\n')
		{
			++index;
		}


		//  주석 넘기기(//)
		while (Buffer[index] == '/' && Buffer[index + 1] == '/')
		{
			while (1)
			{
				++index;
				if (Buffer[index] == '\r')
				{
					index += 2;
					break;
				}
			}
		}

		// 주석 넘기기(/**/)
		if (Buffer[index] == '/' && Buffer[index + 1] == '*')
		{
			++index;
			while (1)
			{
				++index;
				if (Buffer[index] == '*' && Buffer[index + 1] == '/')
				{
					++index;
					break;
				}
			}
			++index;
		}
	}

	return;
}

bool CTextPasing::GetLoadData(const char* filename)
{
	FILE* pFile;

	// 파일 오픈
	if (fopen_s(&pFile, filename, "rb") != 0) // r은 엔터를 \n으로 읽음 , rb는 엔터를 \r\n 으로 읽음
		return false;

	// 파일 사이즈 검색
	fseek(pFile, 0, SEEK_END);	// 위치 지정자 파일 끝으로 이동
	BufferSize = ftell(pFile);	// 파일 시작부터 현재 파일 지정자 위치까지의 
	fseek(pFile, 0, SEEK_SET);	// 위치 지정자 파일 시작으로 이동

	// 파일 로드
	Buffer = new char[BufferSize];
	fread(Buffer, 1, BufferSize, pFile);

	fclose(pFile);
	return true;
}
bool CTextPasing::GetValueChar(char* pvalue, const char* findword)
{
	char chWord[256] = { 0, };
	size_t WordLen = strlen(findword);
	int DataLen = 0;

	for (int iCnt = 0; iCnt < BufferSize; ++iCnt)
	{
		SkipNone(iCnt);

		// Word 찾기
		if (findword[0] == Buffer[iCnt])
		{
			memcpy(chWord, &Buffer[iCnt], WordLen);
			if (0 == strcmp(findword, chWord))
			{
				iCnt += WordLen;
				//---------------------------------------------------------
				// 만약에 find 단어가 E_Atk인데 
				// E_AtkShape 라는 단어가 먼저 있으면
				// E_Atk을 찾긴 하지만 (E_Atk)Shape ()부분을 찾게 된다
				// 그러므로 찾는 단어 다음에 공백문자나 /가 안오면 버그가 생긴다
				// 그래서 공백문자나 /가 아니면 찾는 단어가 아닌것이다.
				//---------------------------------------------------------
				if (Buffer[iCnt] != ' ' && Buffer[iCnt] != '\t' && Buffer[iCnt] != '\r' && Buffer[iCnt] != '/')
				{
					iCnt -= WordLen;
					continue;
				}
				SkipNone(iCnt);

				if (Buffer[iCnt] == '=')
				{
					++iCnt;
					SkipNone(iCnt);

					if (Buffer[iCnt] == '\"')
					{
						while (1)
						{
							++iCnt;
							if (Buffer[iCnt] == '\"')
								break;
							++DataLen;
						}

						memset(pvalue, 0, DataLen);
						memcpy(pvalue, &Buffer[iCnt - DataLen], DataLen);

						return true;
					}

					if (Buffer[iCnt] == '\'')
					{
						while (1)
						{
							++iCnt;
							if (Buffer[iCnt] == '\'')
								break;
							++DataLen;
						}
						
						memcpy(pvalue, &Buffer[iCnt - DataLen], DataLen);

						return true;
					}

					return false;
				}
				return false;
			}
		}
	}
	return false;
}
bool CTextPasing::GetValueWChar(wchar_t* pValue, const char* findword)
{
	char chWord[256] = { 0, };
	size_t WordLen = strlen(findword);

	string str;
	wstring wstr;
	int DataLen = 0;

	for (int iCnt = 0; iCnt < BufferSize; ++iCnt)
	{
		SkipNone(iCnt);

		// Word 찾기
		if (findword[0] == Buffer[iCnt])
		{
			memcpy(chWord, &Buffer[iCnt], WordLen);
			if (0 == strcmp(findword, chWord))
			{
				iCnt += WordLen;
				//---------------------------------------------------------
				// 만약에 find 단어가 E_Atk인데 
				// E_AtkShape 라는 단어가 먼저 있으면
				// E_Atk을 찾긴 하지만 (E_Atk)Shape ()부분을 찾게 된다
				// 그러므로 찾는 단어 다음에 공백문자나 /가 안오면 버그가 생긴다
				// 그래서 공백문자나 /가 아니면 찾는 단어가 아닌것이다.
				//---------------------------------------------------------
				if (Buffer[iCnt] != ' ' && Buffer[iCnt] != '\t' && Buffer[iCnt] != '\r' && Buffer[iCnt] != '/')
				{
					iCnt -= WordLen;
					continue;
				}
				SkipNone(iCnt);

				if (Buffer[iCnt] == '=')
				{
					++iCnt;
					SkipNone(iCnt);

					if (Buffer[iCnt] == '\"')
					{
						while (1)
						{
							++iCnt;
							if (Buffer[iCnt] == '\"')
								break;
							++DataLen;
						}

						str.assign(&Buffer[iCnt - DataLen], DataLen);
						wstr.assign(str.begin(), str.end());
						wmemcpy(pValue, wstr.data(), wstr.size());
						return true;
					}

					if (Buffer[iCnt] == '\'')
					{
						while (1)
						{
							++iCnt;
							if (Buffer[iCnt] == '\'')
								break;
							++DataLen;
						}

						str.assign(&Buffer[iCnt - DataLen], DataLen);
						wstr.assign(str.begin(), str.end());
						wmemcpy(pValue, wstr.data(), wstr.size());
						return true;
					}

					return false;
				}
				return false;
			}
		}
	}
	return false;
}
bool CTextPasing::GetValueInt(int* pvalue, const char* findword)
{
	char chWord[256];
	size_t WordLen = strlen(findword);
	int DataLen = 0;

	// chWord 0으로 초기화
	memset(chWord, 0, 256);


	for (int iCnt = 0; iCnt < BufferSize; ++iCnt)
	{
		SkipNone(iCnt);

		// Word 찾기
		if (findword[0] == Buffer[iCnt])
		{
			memcpy(chWord, &Buffer[iCnt], WordLen); // 끝에 '\0' 을 안붙여줘도 되나? => 위에서 memset으로 0으로 초기화 하였기 떄문 상관 없음
			if (0 == strcmp(findword, chWord))
			{
				iCnt += WordLen;
				//---------------------------------------------------------
				// 만약에 find 단어가 E_Atk인데 
				// E_AtkShape 라는 단어가 먼저 있으면
				// E_Atk을 찾긴 하지만 (E_Atk)Shape ()부분을 찾게 된다
				// 그러므로 찾는 단어 다음에 공백문자나 /가 안오면 버그가 생긴다
				// 그래서 공백문자나 /가 아니면 찾는 단어가 아닌것이다.
				//---------------------------------------------------------
				if (Buffer[iCnt] != ' ' && Buffer[iCnt] != '\t' && Buffer[iCnt] != '\r' && Buffer[iCnt] != '/')
				{
					iCnt -= WordLen;
					continue;
				}
				SkipNone(iCnt);

				if (Buffer[iCnt] == '=')
				{
					++iCnt;
					SkipNone(iCnt);
					while (Buffer[iCnt] >= '0' && Buffer[iCnt] <= '9')
					{
						++iCnt;
						++DataLen;
					}
					memset(chWord, 0, 256);
					memcpy(chWord, &Buffer[iCnt - DataLen], DataLen);
					*pvalue = atoi(chWord);

					return true;
				}
				return false;
			}
		}
	}
	return false;
}
bool CTextPasing::GetValueShort(short* pvalue, const char* findword)
{
	char chWord[256];
	size_t WordLen = strlen(findword);
	int DataLen = 0;

	// chWord 0으로 초기화
	memset(chWord, 0, 256);


	for (int iCnt = 0; iCnt < BufferSize; ++iCnt)
	{
		SkipNone(iCnt);

		// Word 찾기
		if (findword[0] == Buffer[iCnt])
		{
			memcpy(chWord, &Buffer[iCnt], WordLen); // 끝에 '\0' 을 안붙여줘도 되나? => 위에서 memset으로 0으로 초기화 하였기 떄문 상관 없음
			if (0 == strcmp(findword, chWord))
			{
				iCnt += WordLen;
				//---------------------------------------------------------
				// 만약에 find 단어가 E_Atk인데 
				// E_AtkShape 라는 단어가 먼저 있으면
				// E_Atk을 찾긴 하지만 (E_Atk)Shape ()부분을 찾게 된다
				// 그러므로 찾는 단어 다음에 공백문자나 /가 안오면 버그가 생긴다
				// 그래서 공백문자나 /가 아니면 찾는 단어가 아닌것이다.
				//---------------------------------------------------------
				if (Buffer[iCnt] != ' ' && Buffer[iCnt] != '\t' && Buffer[iCnt] != '\r' && Buffer[iCnt] != '/')
				{
					iCnt -= WordLen;
					continue;
				}
				SkipNone(iCnt);

				if (Buffer[iCnt] == '=')
				{
					++iCnt;
					SkipNone(iCnt);
					while (Buffer[iCnt] >= '0' && Buffer[iCnt] <= '9')
					{
						++iCnt;
						++DataLen;
					}
					memset(chWord, 0, 256);
					memcpy(chWord, &Buffer[iCnt - DataLen], DataLen);
					*pvalue = atoi(chWord);

					return true;
				}
				return false;
			}
		}
	}
	return false;
}
bool CTextPasing::GetValueByte(char* pvalue, const char* findword)
{
	char chWord[256];
	size_t WordLen = strlen(findword);
	int DataLen = 0;

	// chWord 0으로 초기화
	memset(chWord, 0, 256);


	for (int iCnt = 0; iCnt < BufferSize; ++iCnt)
	{
		SkipNone(iCnt);

		// Word 찾기
		if (findword[0] == Buffer[iCnt])
		{
			memcpy(chWord, &Buffer[iCnt], WordLen); // 끝에 '\0' 을 안붙여줘도 되나? => 위에서 memset으로 0으로 초기화 하였기 떄문 상관 없음
			if (0 == strcmp(findword, chWord))
			{
				iCnt += WordLen;
				//---------------------------------------------------------
				// 만약에 find 단어가 E_Atk인데 
				// E_AtkShape 라는 단어가 먼저 있으면
				// E_Atk을 찾긴 하지만 (E_Atk)Shape ()부분을 찾게 된다
				// 그러므로 찾는 단어 다음에 공백문자나 /가 안오면 버그가 생긴다
				// 그래서 공백문자나 /가 아니면 찾는 단어가 아닌것이다.
				//---------------------------------------------------------
				if (Buffer[iCnt] != ' ' && Buffer[iCnt] != '\t' && Buffer[iCnt] != '\r' && Buffer[iCnt] != '/')
				{
					iCnt -= WordLen;
					continue;
				}
				SkipNone(iCnt);

				if (Buffer[iCnt] == '=')
				{
					++iCnt;
					SkipNone(iCnt);
					while (Buffer[iCnt] >= '0' && Buffer[iCnt] <= '9')
					{
						++iCnt;
						++DataLen;
					}
					memset(chWord, 0, 256);
					memcpy(chWord, &Buffer[iCnt - DataLen], DataLen);
					*pvalue = atoi(chWord);

					return true;
				}
				return false;
			}
		}
	}
	return false;
}
bool CTextPasing::GetValueBool(bool* pValue, const char* findword)
{
	char chWord[256] = { 0, };
	size_t WordLen = strlen(findword);
	int DataLen = 0;

	for (int iCnt = 0; iCnt < BufferSize; ++iCnt)
	{
		SkipNone(iCnt);

		// Word 찾기
		if (findword[0] == Buffer[iCnt])
		{
			memcpy(chWord, &Buffer[iCnt], WordLen);
			if (0 == strcmp(findword, chWord))
			{
				iCnt += WordLen;
				//---------------------------------------------------------
				// 만약에 find 단어가 E_Atk인데 
				// E_AtkShape 라는 단어가 먼저 있으면
				// E_Atk을 찾긴 하지만 (E_Atk)Shape ()부분을 찾게 된다
				// 그러므로 찾는 단어 다음에 공백문자나 /가 안오면 버그가 생긴다
				// 그래서 공백문자나 /가 아니면 찾는 단어가 아닌것이다.
				//---------------------------------------------------------
				if (Buffer[iCnt] != ' ' && Buffer[iCnt] != '\t' && Buffer[iCnt] != '\r' && Buffer[iCnt] != '/')
				{
					iCnt -= WordLen;
					continue;
				}
				SkipNone(iCnt);

				if (Buffer[iCnt] == '=')
				{
					++iCnt;
					SkipNone(iCnt);

					if (Buffer[iCnt] == 'f' || Buffer[iCnt] == 'F')
					{
						*pValue = false;
						return true;
					}

					if (Buffer[iCnt] == 't' || Buffer[iCnt] == 'T')
					{
						*pValue = true;
						return true;
					}

					return false;
				}
				return false;
			}
		}
	}
	return false;
}
bool CTextPasing::GetValueChar(string& str, const char* findword)
{
	char chWord[256] = { 0, };
	size_t WordLen = strlen(findword);
	int DataLen = 0;

	for (int iCnt = 0; iCnt < BufferSize; ++iCnt)
	{
		SkipNone(iCnt);

		// Word 찾기
		if (findword[0] == Buffer[iCnt])
		{
			memcpy(chWord, &Buffer[iCnt], WordLen);
			if (0 == strcmp(findword, chWord))
			{
				iCnt += WordLen;
				//---------------------------------------------------------
				// 만약에 find 단어가 E_Atk인데 
				// E_AtkShape 라는 단어가 먼저 있으면
				// E_Atk을 찾긴 하지만 (E_Atk)Shape ()부분을 찾게 된다
				// 그러므로 찾는 단어 다음에 공백문자나 /가 안오면 버그가 생긴다
				// 그래서 공백문자나 /가 아니면 찾는 단어가 아닌것이다.
				//---------------------------------------------------------
				if (Buffer[iCnt] != ' ' && Buffer[iCnt] != '\t' && Buffer[iCnt] != '\r' && Buffer[iCnt] != '/')
				{
					iCnt -= WordLen;
					continue;
				}
				SkipNone(iCnt);

				if (Buffer[iCnt] == '=')
				{
					++iCnt;
					SkipNone(iCnt);

					if (Buffer[iCnt] == '\"')
					{
						while (1)
						{
							++iCnt;
							if (Buffer[iCnt] == '\"')
								break;
							++DataLen;
						}

						str.assign(&Buffer[iCnt - DataLen], DataLen);

						return true;
					}

					if (Buffer[iCnt] == '\'')
					{
						while (1)
						{
							++iCnt;
							if (Buffer[iCnt] == '\'')
								break;
							++DataLen;
						}

						str.assign(&Buffer[iCnt - DataLen], DataLen);

						return true;
					}

					return false;
				}
				return false;
			}
		}
	}
	return false;
}
bool CTextPasing::GetValueWChar(wstring& wstr, const char* findword)
{
	char chWord[256] = { 0, };
	size_t WordLen = strlen(findword);

	string str;
	int DataLen = 0;

	for (int iCnt = 0; iCnt < BufferSize; ++iCnt)
	{
		SkipNone(iCnt);

		// Word 찾기
		if (findword[0] == Buffer[iCnt])
		{
			memcpy(chWord, &Buffer[iCnt], WordLen);
			if (0 == strcmp(findword, chWord))
			{
				iCnt += WordLen;
				//---------------------------------------------------------
				// 만약에 find 단어가 E_Atk인데 
				// E_AtkShape 라는 단어가 먼저 있으면
				// E_Atk을 찾긴 하지만 (E_Atk)Shape ()부분을 찾게 된다
				// 그러므로 찾는 단어 다음에 공백문자나 /가 안오면 버그가 생긴다
				// 그래서 공백문자나 /가 아니면 찾는 단어가 아닌것이다.
				//---------------------------------------------------------
				if (Buffer[iCnt] != ' ' && Buffer[iCnt] != '\t' && Buffer[iCnt] != '\r' && Buffer[iCnt] != '/')
				{
					iCnt -= WordLen;
					continue;
				}
				SkipNone(iCnt);

				if (Buffer[iCnt] == '=')
				{
					++iCnt;
					SkipNone(iCnt);

					if (Buffer[iCnt] == '\"')
					{
						while (1)
						{
							++iCnt;
							if (Buffer[iCnt] == '\"')
								break;
							++DataLen;
						}

						str.assign(&Buffer[iCnt - DataLen], DataLen);
						wstr.assign(str.begin(), str.end());

						return true;
					}

					if (Buffer[iCnt] == '\'')
					{
						while (1)
						{
							++iCnt;
							if (Buffer[iCnt] == '\'')
								break;
							++DataLen;
						}

						str.assign(&Buffer[iCnt - DataLen], DataLen);
						wstr.assign(str.begin(), str.end());

						return true;
					}

					return false;
				}
				return false;
			}
		}
	}
	return false;
}


//------------------------------------------------------------------------------------------------------------------------------
// CTextPasingCategory
CTextPasingCategory::CTextPasingCategory()
{
	Buffer = nullptr;
	BufferSize = 0;
}
CTextPasingCategory::CTextPasingCategory(char* buffer, int bufferSize)
{
	Buffer = buffer;
	BufferSize = bufferSize;

}

void CTextPasingCategory::SkipNone(int& index)
{
	while (Buffer[index] == ' ' || Buffer[index] == '\t' || Buffer[index] == '\r' || Buffer[index] == '\n' || Buffer[index] == '/')
	{
		// 띄어쓰기(' ') 넘기기 (Space, Tap, Enter)
		while (Buffer[index] == ' ' || Buffer[index] == '\t' || Buffer[index] == '\r' || Buffer[index] == '\n')
		{
			++index;
		}


		//  주석 넘기기(//)
		while (Buffer[index] == '/' && Buffer[index + 1] == '/')
		{
			while (1)
			{
				++index;
				if (Buffer[index] == '\r')
				{
					index += 2;
					break;
				}
			}
		}

		// 주석 넘기기(/**/)
		if (Buffer[index] == '/' && Buffer[index + 1] == '*')
		{
			++index;
			while (1)
			{
				++index;
				if (Buffer[index] == '*' && Buffer[index + 1] == '/')
				{
					++index;
					break;
				}
			}
			++index;
		}
	}
	return;
}

bool CTextPasingCategory::GetValueChar(char* pvalue, const char* findword)
{
	char chWord[256] = { 0, };
	size_t WordLen = strlen(findword);
	int DataLen = 0;

	for (int iCnt = 0; iCnt < BufferSize; ++iCnt)
	{
		SkipNone(iCnt);

		// Word 찾기
		if (findword[0] == Buffer[iCnt])
		{
			memcpy(chWord, &Buffer[iCnt], WordLen);
			if (0 == strcmp(findword, chWord))
			{
				iCnt += WordLen;
				//---------------------------------------------------------
				// 만약에 find 단어가 E_Atk인데 
				// E_AtkShape 라는 단어가 먼저 있으면
				// E_Atk을 찾긴 하지만 (E_Atk)Shape ()부분을 찾게 된다
				// 그러므로 찾는 단어 다음에 공백문자나 /가 안오면 버그가 생긴다
				// 그래서 공백문자나 /가 아니면 찾는 단어가 아닌것이다.
				//---------------------------------------------------------
				if (Buffer[iCnt] != ' ' && Buffer[iCnt] != '\t' && Buffer[iCnt] != '\r' && Buffer[iCnt] != '/')
				{
					iCnt -= WordLen;
					continue;
				}
				SkipNone(iCnt);

				if (Buffer[iCnt] == '=')
				{
					++iCnt;
					SkipNone(iCnt);

					if (Buffer[iCnt] == '\"')
					{
						while (1)
						{
							++iCnt;
							if (Buffer[iCnt] == '\"')
								break;
							++DataLen;
						}

						memset(pvalue, 0, DataLen);
						memcpy(pvalue, &Buffer[iCnt - DataLen], DataLen);

						return true;
					}

					if (Buffer[iCnt] == '\'')
					{
						while (1)
						{
							++iCnt;
							if (Buffer[iCnt] == '\'')
								break;
							++DataLen;
						}

						//memset(pvalue, 0, 256);
						memcpy(pvalue, &Buffer[iCnt - DataLen], DataLen);

						return true;
					}

					return false;
				}
				return false;
			}
		}
	}
	return false;
}
bool CTextPasingCategory::GetValueWChar(wchar_t* pValue, const char* findword)
{
	char chWord[256] = { 0, };
	size_t WordLen = strlen(findword);

	string str;
	wstring wstr;
	int DataLen = 0;

	for (int iCnt = 0; iCnt < BufferSize; ++iCnt)
	{
		SkipNone(iCnt);

		// Word 찾기
		if (findword[0] == Buffer[iCnt])
		{
			memcpy(chWord, &Buffer[iCnt], WordLen);
			if (0 == strcmp(findword, chWord))
			{
				iCnt += WordLen;
				//---------------------------------------------------------
				// 만약에 find 단어가 E_Atk인데 
				// E_AtkShape 라는 단어가 먼저 있으면
				// E_Atk을 찾긴 하지만 (E_Atk)Shape ()부분을 찾게 된다
				// 그러므로 찾는 단어 다음에 공백문자나 /가 안오면 버그가 생긴다
				// 그래서 공백문자나 /가 아니면 찾는 단어가 아닌것이다.
				//---------------------------------------------------------
				if (Buffer[iCnt] != ' ' && Buffer[iCnt] != '\t' && Buffer[iCnt] != '\r' && Buffer[iCnt] != '/')
				{
					iCnt -= WordLen;
					continue;
				}
				SkipNone(iCnt);

				if (Buffer[iCnt] == '=')
				{
					++iCnt;
					SkipNone(iCnt);

					if (Buffer[iCnt] == '\"')
					{
						while (1)
						{
							++iCnt;
							if (Buffer[iCnt] == '\"')
								break;
							++DataLen;
						}

						str.assign(&Buffer[iCnt - DataLen], DataLen);
						wstr.assign(str.begin(), str.end());
						wmemcpy(pValue, wstr.data(), wstr.size());
						return true;
					}

					if (Buffer[iCnt] == '\'')
					{
						while (1)
						{
							++iCnt;
							if (Buffer[iCnt] == '\'')
								break;
							++DataLen;
						}

						str.assign(&Buffer[iCnt - DataLen], DataLen);
						wstr.assign(str.begin(), str.end());
						wmemcpy(pValue, wstr.data(), wstr.size());
						return true;
					}

					return false;
				}
				return false;
			}
		}
	}
	return false;
}
bool CTextPasingCategory::GetValueInt(int* pvalue, const char* findword)
{
	char chWord[256];
	size_t WordLen = strlen(findword);
	int DataLen = 0;

	// chWord 0으로 초기화
	memset(chWord, 0, 256);


	for (int iCnt = 0; iCnt < BufferSize; ++iCnt)
	{
		SkipNone(iCnt);

		// Word 찾기
		if (findword[0] == Buffer[iCnt])
		{
			memcpy(chWord, &Buffer[iCnt], WordLen); // 끝에 '\0' 을 안붙여줘도 되나? => 위에서 memset으로 0으로 초기화 하였기 떄문 상관 없음
			if (0 == strcmp(findword, chWord))
			{
				iCnt += WordLen;
				//---------------------------------------------------------
				// 만약에 find 단어가 E_Atk인데 
				// E_AtkShape 라는 단어가 먼저 있으면
				// E_Atk을 찾긴 하지만 (E_Atk)Shape ()부분을 찾게 된다
				// 그러므로 찾는 단어 다음에 공백문자나 /가 안오면 버그가 생긴다
				// 그래서 공백문자나 /가 아니면 찾는 단어가 아닌것이다.
				//---------------------------------------------------------
				if (Buffer[iCnt] != ' ' && Buffer[iCnt] != '\t' && Buffer[iCnt] != '\r' && Buffer[iCnt] != '/')
				{
					iCnt -= WordLen;
					continue;
				}
				SkipNone(iCnt);

				if (Buffer[iCnt] == '=')
				{
					++iCnt;
					SkipNone(iCnt);
					while (Buffer[iCnt] >= '0' && Buffer[iCnt] <= '9')
					{
						++iCnt;
						++DataLen;
					}
					memset(chWord, 0, 256);
					memcpy(chWord, &Buffer[iCnt - DataLen], DataLen);
					*pvalue = atoi(chWord);

					return true;
				}
				return false;
			}
		}
	}
	return false;
}
bool CTextPasingCategory::GetValueShort(short* pvalue, const char* findword)
{
	char chWord[256];
	size_t WordLen = strlen(findword);
	int DataLen = 0;

	// chWord 0으로 초기화
	memset(chWord, 0, 256);


	for (int iCnt = 0; iCnt < BufferSize; ++iCnt)
	{
		SkipNone(iCnt);

		// Word 찾기
		if (findword[0] == Buffer[iCnt])
		{
			memcpy(chWord, &Buffer[iCnt], WordLen); // 끝에 '\0' 을 안붙여줘도 되나? => 위에서 memset으로 0으로 초기화 하였기 떄문 상관 없음
			if (0 == strcmp(findword, chWord))
			{
				iCnt += WordLen;
				//---------------------------------------------------------
				// 만약에 find 단어가 E_Atk인데 
				// E_AtkShape 라는 단어가 먼저 있으면
				// E_Atk을 찾긴 하지만 (E_Atk)Shape ()부분을 찾게 된다
				// 그러므로 찾는 단어 다음에 공백문자나 /가 안오면 버그가 생긴다
				// 그래서 공백문자나 /가 아니면 찾는 단어가 아닌것이다.
				//---------------------------------------------------------
				if (Buffer[iCnt] != ' ' && Buffer[iCnt] != '\t' && Buffer[iCnt] != '\r' && Buffer[iCnt] != '/')
				{
					iCnt -= WordLen;
					continue;
				}
				SkipNone(iCnt);

				if (Buffer[iCnt] == '=')
				{
					++iCnt;
					SkipNone(iCnt);
					while (Buffer[iCnt] >= '0' && Buffer[iCnt] <= '9')
					{
						++iCnt;
						++DataLen;
					}
					memset(chWord, 0, 256);
					memcpy(chWord, &Buffer[iCnt - DataLen], DataLen);
					*pvalue = atoi(chWord);

					return true;
				}
				return false;
			}
		}
	}
	return false;
}
bool CTextPasingCategory::GetValueByte(char* pvalue, const char* findword)
{
	char chWord[256];
	size_t WordLen = strlen(findword);
	int DataLen = 0;

	// chWord 0으로 초기화
	memset(chWord, 0, 256);


	for (int iCnt = 0; iCnt < BufferSize; ++iCnt)
	{
		SkipNone(iCnt);

		// Word 찾기
		if (findword[0] == Buffer[iCnt])
		{
			memcpy(chWord, &Buffer[iCnt], WordLen); // 끝에 '\0' 을 안붙여줘도 되나? => 위에서 memset으로 0으로 초기화 하였기 떄문 상관 없음
			if (0 == strcmp(findword, chWord))
			{
				iCnt += WordLen;
				//---------------------------------------------------------
				// 만약에 find 단어가 E_Atk인데 
				// E_AtkShape 라는 단어가 먼저 있으면
				// E_Atk을 찾긴 하지만 (E_Atk)Shape ()부분을 찾게 된다
				// 그러므로 찾는 단어 다음에 공백문자나 /가 안오면 버그가 생긴다
				// 그래서 공백문자나 /가 아니면 찾는 단어가 아닌것이다.
				//---------------------------------------------------------
				if (Buffer[iCnt] != ' ' && Buffer[iCnt] != '\t' && Buffer[iCnt] != '\r' && Buffer[iCnt] != '/')
				{
					iCnt -= WordLen;
					continue;
				}
				SkipNone(iCnt);

				if (Buffer[iCnt] == '=')
				{
					++iCnt;
					SkipNone(iCnt);
					while (Buffer[iCnt] >= '0' && Buffer[iCnt] <= '9')
					{
						++iCnt;
						++DataLen;
					}
					memset(chWord, 0, 256);
					memcpy(chWord, &Buffer[iCnt - DataLen], DataLen);
					*pvalue = atoi(chWord);

					return true;
				}
				return false;
			}
		}
	}
	return false;
}
bool CTextPasingCategory::GetValueBool(bool* pValue, const char* findword)
{
	char chWord[256] = { 0, };
	size_t WordLen = strlen(findword);
	int DataLen = 0;

	for (int iCnt = 0; iCnt < BufferSize; ++iCnt)
	{
		SkipNone(iCnt);

		// Word 찾기
		if (findword[0] == Buffer[iCnt])
		{
			memcpy(chWord, &Buffer[iCnt], WordLen);
			if (0 == strcmp(findword, chWord))
			{
				iCnt += WordLen;
				//---------------------------------------------------------
				// 만약에 find 단어가 E_Atk인데 
				// E_AtkShape 라는 단어가 먼저 있으면
				// E_Atk을 찾긴 하지만 (E_Atk)Shape ()부분을 찾게 된다
				// 그러므로 찾는 단어 다음에 공백문자나 /가 안오면 버그가 생긴다
				// 그래서 공백문자나 /가 아니면 찾는 단어가 아닌것이다.
				//---------------------------------------------------------
				if (Buffer[iCnt] != ' ' && Buffer[iCnt] != '\t' && Buffer[iCnt] != '\r' && Buffer[iCnt] != '/')
				{
					iCnt -= WordLen;
					continue;
				}
				SkipNone(iCnt);

				if (Buffer[iCnt] == '=')
				{
					++iCnt;
					SkipNone(iCnt);

					if (Buffer[iCnt] == 'f' || Buffer[iCnt] == 'F')
					{
						*pValue = false;
						return true;
					}

					if (Buffer[iCnt] == 't' || Buffer[iCnt] == 'T')
					{
						*pValue = true;
						return true;
					}

					return false;
				}
				return false;
			}
		}
	}
	return false;
}
bool CTextPasingCategory::GetValueChar(string& str, const char* findword)
{
	char chWord[256] = { 0, };
	size_t WordLen = strlen(findword);
	int DataLen = 0;

	for (int iCnt = 0; iCnt < BufferSize; ++iCnt)
	{
		SkipNone(iCnt);

		// Word 찾기
		if (findword[0] == Buffer[iCnt])
		{
			memcpy(chWord, &Buffer[iCnt], WordLen);
			if (0 == strcmp(findword, chWord))
			{
				iCnt += WordLen;
				//---------------------------------------------------------
				// 만약에 find 단어가 E_Atk인데 
				// E_AtkShape 라는 단어가 먼저 있으면
				// E_Atk을 찾긴 하지만 (E_Atk)Shape ()부분을 찾게 된다
				// 그러므로 찾는 단어 다음에 공백문자나 /가 안오면 버그가 생긴다
				// 그래서 공백문자나 /가 아니면 찾는 단어가 아닌것이다.
				//---------------------------------------------------------
				if (Buffer[iCnt] != ' ' && Buffer[iCnt] != '\t' && Buffer[iCnt] != '\r' && Buffer[iCnt] != '/')
				{
					iCnt -= WordLen;
					continue;
				}
				SkipNone(iCnt);

				if (Buffer[iCnt] == '=')
				{
					++iCnt;
					SkipNone(iCnt);

					if (Buffer[iCnt] == '\"')
					{
						while (1)
						{
							++iCnt;
							if (Buffer[iCnt] == '\"')
								break;
							++DataLen;
						}

						str.assign(&Buffer[iCnt - DataLen], DataLen);

						return true;
					}

					if (Buffer[iCnt] == '\'')
					{
						while (1)
						{
							++iCnt;
							if (Buffer[iCnt] == '\'')
								break;
							++DataLen;
						}

						str.assign(&Buffer[iCnt - DataLen], DataLen);

						return true;
					}

					return false;
				}
				return false;
			}
		}
	}
	return false;
}
bool CTextPasingCategory::GetValueWChar(wstring& wstr, const char* findword)
{
	char chWord[256] = { 0, };
	size_t WordLen = strlen(findword);

	string str;
	int DataLen = 0;

	for (int iCnt = 0; iCnt < BufferSize; ++iCnt)
	{
		SkipNone(iCnt);

		// Word 찾기
		if (findword[0] == Buffer[iCnt])
		{
			memcpy(chWord, &Buffer[iCnt], WordLen);
			if (0 == strcmp(findword, chWord))
			{
				iCnt += WordLen;
				//---------------------------------------------------------
				// 만약에 find 단어가 E_Atk인데 
				// E_AtkShape 라는 단어가 먼저 있으면
				// E_Atk을 찾긴 하지만 (E_Atk)Shape ()부분을 찾게 된다
				// 그러므로 찾는 단어 다음에 공백문자나 /가 안오면 버그가 생긴다
				// 그래서 공백문자나 /가 아니면 찾는 단어가 아닌것이다.
				//---------------------------------------------------------
				if (Buffer[iCnt] != ' ' && Buffer[iCnt] != '\t' && Buffer[iCnt] != '\r' && Buffer[iCnt] != '/')
				{
					iCnt -= WordLen;
					continue;
				}
				SkipNone(iCnt);

				if (Buffer[iCnt] == '=')
				{
					++iCnt;
					SkipNone(iCnt);

					if (Buffer[iCnt] == '\"')
					{
						while (1)
						{
							++iCnt;
							if (Buffer[iCnt] == '\"')
								break;
							++DataLen;
						}

						str.assign(&Buffer[iCnt - DataLen], DataLen);
						wstr.assign(str.begin(), str.end());

						return true;
					}

					if (Buffer[iCnt] == '\'')
					{
						while (1)
						{
							++iCnt;
							if (Buffer[iCnt] == '\'')
								break;
							++DataLen;
						}

						str.assign(&Buffer[iCnt - DataLen], DataLen);
						wstr.assign(str.begin(), str.end());

						return true;
					}

					return false;
				}
				return false;
			}
		}
	}
	return false;
}