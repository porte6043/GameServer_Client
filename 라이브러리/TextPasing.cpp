#include "TextPasing.h"

//---------------------------------------------------
// rb(���̳ʸ����)
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

		// Category ã��
		if (':' == Buffer[iCnt])
		{
			++iCnt;
			SkipNone(iCnt);
			memcpy(CategoryWord, &Buffer[iCnt], CategoryLen);
			if (0 == strcmp(findcategory, CategoryWord))
			{
				iCnt += CategoryLen;
				//---------------------------------------------------------
				// ���࿡ find �ܾ E_Atk�ε� 
				// E_AtkShape ��� �ܾ ���� ������
				// E_Atk�� ã�� ������ (E_Atk)Shape ()�κ��� ã�� �ȴ�
				// �׷��Ƿ� ã�� �ܾ� ������ ���鹮�ڳ� /�� �ȿ��� ���װ� �����
				// �׷��� ���鹮�ڳ� /�� �ƴϸ� ã�� �ܾ �ƴѰ��̴�.
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

					// ���� ��ġ ����
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
		// ����(' ') �ѱ�� (Space, Tap, Enter)
		while (Buffer[index] == ' ' || Buffer[index] == '\t' || Buffer[index] == '\r' || Buffer[index] == '\n')
		{
			++index;
		}


		//  �ּ� �ѱ��(//)
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

		// �ּ� �ѱ��(/**/)
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

	// ���� ����
	if (fopen_s(&pFile, filename, "rb") != 0) // r�� ���͸� \n���� ���� , rb�� ���͸� \r\n ���� ����
		return false;

	// ���� ������ �˻�
	fseek(pFile, 0, SEEK_END);	// ��ġ ������ ���� ������ �̵�
	BufferSize = ftell(pFile);	// ���� ���ۺ��� ���� ���� ������ ��ġ������ 
	fseek(pFile, 0, SEEK_SET);	// ��ġ ������ ���� �������� �̵�

	// ���� �ε�
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

		// Word ã��
		if (findword[0] == Buffer[iCnt])
		{
			memcpy(chWord, &Buffer[iCnt], WordLen);
			if (0 == strcmp(findword, chWord))
			{
				iCnt += WordLen;
				//---------------------------------------------------------
				// ���࿡ find �ܾ E_Atk�ε� 
				// E_AtkShape ��� �ܾ ���� ������
				// E_Atk�� ã�� ������ (E_Atk)Shape ()�κ��� ã�� �ȴ�
				// �׷��Ƿ� ã�� �ܾ� ������ ���鹮�ڳ� /�� �ȿ��� ���װ� �����
				// �׷��� ���鹮�ڳ� /�� �ƴϸ� ã�� �ܾ �ƴѰ��̴�.
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

		// Word ã��
		if (findword[0] == Buffer[iCnt])
		{
			memcpy(chWord, &Buffer[iCnt], WordLen);
			if (0 == strcmp(findword, chWord))
			{
				iCnt += WordLen;
				//---------------------------------------------------------
				// ���࿡ find �ܾ E_Atk�ε� 
				// E_AtkShape ��� �ܾ ���� ������
				// E_Atk�� ã�� ������ (E_Atk)Shape ()�κ��� ã�� �ȴ�
				// �׷��Ƿ� ã�� �ܾ� ������ ���鹮�ڳ� /�� �ȿ��� ���װ� �����
				// �׷��� ���鹮�ڳ� /�� �ƴϸ� ã�� �ܾ �ƴѰ��̴�.
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

	// chWord 0���� �ʱ�ȭ
	memset(chWord, 0, 256);


	for (int iCnt = 0; iCnt < BufferSize; ++iCnt)
	{
		SkipNone(iCnt);

		// Word ã��
		if (findword[0] == Buffer[iCnt])
		{
			memcpy(chWord, &Buffer[iCnt], WordLen); // ���� '\0' �� �Ⱥٿ��൵ �ǳ�? => ������ memset���� 0���� �ʱ�ȭ �Ͽ��� ���� ��� ����
			if (0 == strcmp(findword, chWord))
			{
				iCnt += WordLen;
				//---------------------------------------------------------
				// ���࿡ find �ܾ E_Atk�ε� 
				// E_AtkShape ��� �ܾ ���� ������
				// E_Atk�� ã�� ������ (E_Atk)Shape ()�κ��� ã�� �ȴ�
				// �׷��Ƿ� ã�� �ܾ� ������ ���鹮�ڳ� /�� �ȿ��� ���װ� �����
				// �׷��� ���鹮�ڳ� /�� �ƴϸ� ã�� �ܾ �ƴѰ��̴�.
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

	// chWord 0���� �ʱ�ȭ
	memset(chWord, 0, 256);


	for (int iCnt = 0; iCnt < BufferSize; ++iCnt)
	{
		SkipNone(iCnt);

		// Word ã��
		if (findword[0] == Buffer[iCnt])
		{
			memcpy(chWord, &Buffer[iCnt], WordLen); // ���� '\0' �� �Ⱥٿ��൵ �ǳ�? => ������ memset���� 0���� �ʱ�ȭ �Ͽ��� ���� ��� ����
			if (0 == strcmp(findword, chWord))
			{
				iCnt += WordLen;
				//---------------------------------------------------------
				// ���࿡ find �ܾ E_Atk�ε� 
				// E_AtkShape ��� �ܾ ���� ������
				// E_Atk�� ã�� ������ (E_Atk)Shape ()�κ��� ã�� �ȴ�
				// �׷��Ƿ� ã�� �ܾ� ������ ���鹮�ڳ� /�� �ȿ��� ���װ� �����
				// �׷��� ���鹮�ڳ� /�� �ƴϸ� ã�� �ܾ �ƴѰ��̴�.
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

	// chWord 0���� �ʱ�ȭ
	memset(chWord, 0, 256);


	for (int iCnt = 0; iCnt < BufferSize; ++iCnt)
	{
		SkipNone(iCnt);

		// Word ã��
		if (findword[0] == Buffer[iCnt])
		{
			memcpy(chWord, &Buffer[iCnt], WordLen); // ���� '\0' �� �Ⱥٿ��൵ �ǳ�? => ������ memset���� 0���� �ʱ�ȭ �Ͽ��� ���� ��� ����
			if (0 == strcmp(findword, chWord))
			{
				iCnt += WordLen;
				//---------------------------------------------------------
				// ���࿡ find �ܾ E_Atk�ε� 
				// E_AtkShape ��� �ܾ ���� ������
				// E_Atk�� ã�� ������ (E_Atk)Shape ()�κ��� ã�� �ȴ�
				// �׷��Ƿ� ã�� �ܾ� ������ ���鹮�ڳ� /�� �ȿ��� ���װ� �����
				// �׷��� ���鹮�ڳ� /�� �ƴϸ� ã�� �ܾ �ƴѰ��̴�.
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

		// Word ã��
		if (findword[0] == Buffer[iCnt])
		{
			memcpy(chWord, &Buffer[iCnt], WordLen);
			if (0 == strcmp(findword, chWord))
			{
				iCnt += WordLen;
				//---------------------------------------------------------
				// ���࿡ find �ܾ E_Atk�ε� 
				// E_AtkShape ��� �ܾ ���� ������
				// E_Atk�� ã�� ������ (E_Atk)Shape ()�κ��� ã�� �ȴ�
				// �׷��Ƿ� ã�� �ܾ� ������ ���鹮�ڳ� /�� �ȿ��� ���װ� �����
				// �׷��� ���鹮�ڳ� /�� �ƴϸ� ã�� �ܾ �ƴѰ��̴�.
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

		// Word ã��
		if (findword[0] == Buffer[iCnt])
		{
			memcpy(chWord, &Buffer[iCnt], WordLen);
			if (0 == strcmp(findword, chWord))
			{
				iCnt += WordLen;
				//---------------------------------------------------------
				// ���࿡ find �ܾ E_Atk�ε� 
				// E_AtkShape ��� �ܾ ���� ������
				// E_Atk�� ã�� ������ (E_Atk)Shape ()�κ��� ã�� �ȴ�
				// �׷��Ƿ� ã�� �ܾ� ������ ���鹮�ڳ� /�� �ȿ��� ���װ� �����
				// �׷��� ���鹮�ڳ� /�� �ƴϸ� ã�� �ܾ �ƴѰ��̴�.
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

		// Word ã��
		if (findword[0] == Buffer[iCnt])
		{
			memcpy(chWord, &Buffer[iCnt], WordLen);
			if (0 == strcmp(findword, chWord))
			{
				iCnt += WordLen;
				//---------------------------------------------------------
				// ���࿡ find �ܾ E_Atk�ε� 
				// E_AtkShape ��� �ܾ ���� ������
				// E_Atk�� ã�� ������ (E_Atk)Shape ()�κ��� ã�� �ȴ�
				// �׷��Ƿ� ã�� �ܾ� ������ ���鹮�ڳ� /�� �ȿ��� ���װ� �����
				// �׷��� ���鹮�ڳ� /�� �ƴϸ� ã�� �ܾ �ƴѰ��̴�.
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
		// ����(' ') �ѱ�� (Space, Tap, Enter)
		while (Buffer[index] == ' ' || Buffer[index] == '\t' || Buffer[index] == '\r' || Buffer[index] == '\n')
		{
			++index;
		}


		//  �ּ� �ѱ��(//)
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

		// �ּ� �ѱ��(/**/)
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

		// Word ã��
		if (findword[0] == Buffer[iCnt])
		{
			memcpy(chWord, &Buffer[iCnt], WordLen);
			if (0 == strcmp(findword, chWord))
			{
				iCnt += WordLen;
				//---------------------------------------------------------
				// ���࿡ find �ܾ E_Atk�ε� 
				// E_AtkShape ��� �ܾ ���� ������
				// E_Atk�� ã�� ������ (E_Atk)Shape ()�κ��� ã�� �ȴ�
				// �׷��Ƿ� ã�� �ܾ� ������ ���鹮�ڳ� /�� �ȿ��� ���װ� �����
				// �׷��� ���鹮�ڳ� /�� �ƴϸ� ã�� �ܾ �ƴѰ��̴�.
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

		// Word ã��
		if (findword[0] == Buffer[iCnt])
		{
			memcpy(chWord, &Buffer[iCnt], WordLen);
			if (0 == strcmp(findword, chWord))
			{
				iCnt += WordLen;
				//---------------------------------------------------------
				// ���࿡ find �ܾ E_Atk�ε� 
				// E_AtkShape ��� �ܾ ���� ������
				// E_Atk�� ã�� ������ (E_Atk)Shape ()�κ��� ã�� �ȴ�
				// �׷��Ƿ� ã�� �ܾ� ������ ���鹮�ڳ� /�� �ȿ��� ���װ� �����
				// �׷��� ���鹮�ڳ� /�� �ƴϸ� ã�� �ܾ �ƴѰ��̴�.
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

	// chWord 0���� �ʱ�ȭ
	memset(chWord, 0, 256);


	for (int iCnt = 0; iCnt < BufferSize; ++iCnt)
	{
		SkipNone(iCnt);

		// Word ã��
		if (findword[0] == Buffer[iCnt])
		{
			memcpy(chWord, &Buffer[iCnt], WordLen); // ���� '\0' �� �Ⱥٿ��൵ �ǳ�? => ������ memset���� 0���� �ʱ�ȭ �Ͽ��� ���� ��� ����
			if (0 == strcmp(findword, chWord))
			{
				iCnt += WordLen;
				//---------------------------------------------------------
				// ���࿡ find �ܾ E_Atk�ε� 
				// E_AtkShape ��� �ܾ ���� ������
				// E_Atk�� ã�� ������ (E_Atk)Shape ()�κ��� ã�� �ȴ�
				// �׷��Ƿ� ã�� �ܾ� ������ ���鹮�ڳ� /�� �ȿ��� ���װ� �����
				// �׷��� ���鹮�ڳ� /�� �ƴϸ� ã�� �ܾ �ƴѰ��̴�.
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

	// chWord 0���� �ʱ�ȭ
	memset(chWord, 0, 256);


	for (int iCnt = 0; iCnt < BufferSize; ++iCnt)
	{
		SkipNone(iCnt);

		// Word ã��
		if (findword[0] == Buffer[iCnt])
		{
			memcpy(chWord, &Buffer[iCnt], WordLen); // ���� '\0' �� �Ⱥٿ��൵ �ǳ�? => ������ memset���� 0���� �ʱ�ȭ �Ͽ��� ���� ��� ����
			if (0 == strcmp(findword, chWord))
			{
				iCnt += WordLen;
				//---------------------------------------------------------
				// ���࿡ find �ܾ E_Atk�ε� 
				// E_AtkShape ��� �ܾ ���� ������
				// E_Atk�� ã�� ������ (E_Atk)Shape ()�κ��� ã�� �ȴ�
				// �׷��Ƿ� ã�� �ܾ� ������ ���鹮�ڳ� /�� �ȿ��� ���װ� �����
				// �׷��� ���鹮�ڳ� /�� �ƴϸ� ã�� �ܾ �ƴѰ��̴�.
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

	// chWord 0���� �ʱ�ȭ
	memset(chWord, 0, 256);


	for (int iCnt = 0; iCnt < BufferSize; ++iCnt)
	{
		SkipNone(iCnt);

		// Word ã��
		if (findword[0] == Buffer[iCnt])
		{
			memcpy(chWord, &Buffer[iCnt], WordLen); // ���� '\0' �� �Ⱥٿ��൵ �ǳ�? => ������ memset���� 0���� �ʱ�ȭ �Ͽ��� ���� ��� ����
			if (0 == strcmp(findword, chWord))
			{
				iCnt += WordLen;
				//---------------------------------------------------------
				// ���࿡ find �ܾ E_Atk�ε� 
				// E_AtkShape ��� �ܾ ���� ������
				// E_Atk�� ã�� ������ (E_Atk)Shape ()�κ��� ã�� �ȴ�
				// �׷��Ƿ� ã�� �ܾ� ������ ���鹮�ڳ� /�� �ȿ��� ���װ� �����
				// �׷��� ���鹮�ڳ� /�� �ƴϸ� ã�� �ܾ �ƴѰ��̴�.
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

		// Word ã��
		if (findword[0] == Buffer[iCnt])
		{
			memcpy(chWord, &Buffer[iCnt], WordLen);
			if (0 == strcmp(findword, chWord))
			{
				iCnt += WordLen;
				//---------------------------------------------------------
				// ���࿡ find �ܾ E_Atk�ε� 
				// E_AtkShape ��� �ܾ ���� ������
				// E_Atk�� ã�� ������ (E_Atk)Shape ()�κ��� ã�� �ȴ�
				// �׷��Ƿ� ã�� �ܾ� ������ ���鹮�ڳ� /�� �ȿ��� ���װ� �����
				// �׷��� ���鹮�ڳ� /�� �ƴϸ� ã�� �ܾ �ƴѰ��̴�.
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

		// Word ã��
		if (findword[0] == Buffer[iCnt])
		{
			memcpy(chWord, &Buffer[iCnt], WordLen);
			if (0 == strcmp(findword, chWord))
			{
				iCnt += WordLen;
				//---------------------------------------------------------
				// ���࿡ find �ܾ E_Atk�ε� 
				// E_AtkShape ��� �ܾ ���� ������
				// E_Atk�� ã�� ������ (E_Atk)Shape ()�κ��� ã�� �ȴ�
				// �׷��Ƿ� ã�� �ܾ� ������ ���鹮�ڳ� /�� �ȿ��� ���װ� �����
				// �׷��� ���鹮�ڳ� /�� �ƴϸ� ã�� �ܾ �ƴѰ��̴�.
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

		// Word ã��
		if (findword[0] == Buffer[iCnt])
		{
			memcpy(chWord, &Buffer[iCnt], WordLen);
			if (0 == strcmp(findword, chWord))
			{
				iCnt += WordLen;
				//---------------------------------------------------------
				// ���࿡ find �ܾ E_Atk�ε� 
				// E_AtkShape ��� �ܾ ���� ������
				// E_Atk�� ã�� ������ (E_Atk)Shape ()�κ��� ã�� �ȴ�
				// �׷��Ƿ� ã�� �ܾ� ������ ���鹮�ڳ� /�� �ȿ��� ���װ� �����
				// �׷��� ���鹮�ڳ� /�� �ƴϸ� ã�� �ܾ �ƴѰ��̴�.
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