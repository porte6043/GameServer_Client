#include "TLSProfiling.h"



CTlsProfiling::CTlsProfiling() : TAG{}, ThreadID{}, TagNameIndex(-1)
{
	timeBeginPeriod(1);

	TlsIndex = TlsAlloc();
	if (TlsIndex == TLS_OUT_OF_INDEXES)
	{
		printf("Tls Lack...\n");
		*(int*)0 = 0;
	}
}

CTlsProfiling::~CTlsProfiling()
{
	timeEndPeriod(0);
}

CTlsProfiling* CTlsProfiling::GetInstance()
{
	static CTlsProfiling TlsProfiling;
	return &TlsProfiling;
}


void CTlsProfiling::ProfileBegin(const WCHAR* tagname)
{
	TAG_NAME* Tag = (TAG_NAME*)TlsGetValue(TlsIndex);
	if (Tag == NULL)
	{
		DWORD TagIndex = InterlockedIncrement(&TagNameIndex);
		TlsSetValue(TlsIndex, &TAG[TagIndex][0]);
		ThreadID[TagIndex] = GetCurrentThreadId();
		Tag = &TAG[TagIndex][0];
	}

	// tagname�� �ش��ϴ� Sample�� �ִ��� Ȯ��
	for (int Index = 0; Index < TAGARRCOUNT; ++Index)
	{
		if (wcscmp(Tag[Index]._TagName, tagname) == 0)
		{
			QueryPerformanceCounter(&Tag[Index]._StartTime);
			return;
		}
	}

	// Sample ����
	for (int Index = 0; Index < TAGARRCOUNT; ++Index)
	{
		// ����ִ� Index ã��
		if (Tag[Index]._Flag != 0) continue;

		// ProfileSample �ʱ�ȭ
		Tag[Index]._Flag = 1;
		for (int iMin = 0; iMin < 3; ++iMin)
		{
			Tag[Index]._Min[iMin] = 0x7fffffffffffffff;
		}
		wcscpy_s(Tag[Index]._TagName, 64, tagname);
		QueryPerformanceCounter(&Tag[Index]._StartTime);

		return;
	}

	printf("TAG_NAME Lack...\n");
	*(int*)0 = 0;
}

void CTlsProfiling::ProfileEnd(const WCHAR* tagname)
{
	LARGE_INTEGER EndTime;
	__int64 DeltaTime;

	// �����̶� ��Ȯ�� �����ϱ� ���� tagname�� �ִ� ���� ���� �ð� ���� ����
	QueryPerformanceCounter(&EndTime);

	TAG_NAME* Tag = (TAG_NAME*)TlsGetValue(TlsIndex);

	// tagname�� �ش��ϴ� Sample�� �ִ��� Ȯ��
	int Index = 0;
	for (; Index < TAGARRCOUNT; ++Index)
	{
		if (wcscmp(Tag[Index]._TagName, tagname) != 0)
			continue;
		else
			break;
	}

	// tagname�� �ش��ϴ� Sample�� ������ �Ʒ� ����ó��
	DeltaTime = EndTime.QuadPart - Tag[Index]._StartTime.QuadPart;
	Tag[Index]._TotalTime += DeltaTime;


	// _Min���� ���� �� ���� �ð� ã�Ƽ� ����
	int TempMaxIndex = 0;						// _Min���� ���� �� ���� �ð��� �ε���
	__int64 TempMin = 0;
	for (int iCt = 0; iCt < 3; ++iCt)
	{
		if (Tag[Index]._Min[iCt] > TempMin)
		{
			TempMaxIndex = iCt;
			TempMin = Tag[Index]._Min[iCt];
		}
	}
	if (Tag[Index]._Min[TempMaxIndex] > DeltaTime)
	{
		Tag[Index]._Min[TempMaxIndex] = DeltaTime;
	}


	// _Max���� ���� ���� ���� �ð� ã�Ƽ� ����
	int TempMinIndex = 0;						// _Max���� ���� ª�� ���� �ð��� �ε���
	__int64 TempMax = Tag[Index]._Max[0];	// ù �񱳴� _Max[0]
	for (int iCt = 0; iCt < 3; ++iCt)
	{
		if (Tag[Index]._Max[iCt] < TempMax)
		{
			TempMaxIndex = iCt;
			TempMax = Tag[Index]._Max[iCt];
		}
	}
	if (Tag[Index]._Max[TempMaxIndex] < DeltaTime)
	{
		Tag[Index]._Max[TempMaxIndex] = DeltaTime;
	}

	++Tag[Index]._Call;

	return;
}

void CTlsProfiling::ProfileDataOutText(const WCHAR* TextFileName)
{
	float Average;
	float Min;
	float Max;
	float Sum = 0;

	FILE* pFile;
	errno_t errorno;
	while ((errorno = _wfopen_s(&pFile, TextFileName, L"w, ccs=UNICODE")) != 0) {}

	for (int ThreadIndex = 0; ThreadIndex < TagNameIndex + 1; ++ThreadIndex)
	{
		// txt���Ͽ� ����
		fwprintf_s(pFile, L"%-24s|%24s|%24s|%24s|%24s|\n", L"Name", L"Average", L"Min", L"Max", L"Call");
		fwprintf_s(pFile, L"TheadID:%d----------------------------------------------------------------------------------------------------------------\n", ThreadID[ThreadIndex]);
		for (int TagIndex = 0; TagIndex < TAGARRCOUNT; ++TagIndex)
		{
			// _Flag�� 0�� �� �н�
			if (TAG[ThreadIndex][TagIndex]._Flag != 1) continue;

			// Average ���ϱ�
			Sum = 0;
			for (int i = 0; i < 3; ++i)
			{
				Sum += TAG[ThreadIndex][TagIndex]._Min[i];
				Sum += TAG[ThreadIndex][TagIndex]._Max[i];
			}
			Average = (float)(TAG[ThreadIndex][TagIndex]._TotalTime - Sum) / (TAG[ThreadIndex][TagIndex]._Call - 6) / 10;	 // ����ũ�� ����

			// Min ���ϱ�
			Min = TAG[ThreadIndex][TagIndex]._Min[0];
			for (int i = 0; i < 3; ++i)
			{
				if (TAG[ThreadIndex][TagIndex]._Min[i] < Min)
				{
					Min = (float)TAG[ThreadIndex][TagIndex]._Min[i] / 10;		// ����ũ�� ����
				}
			}

			// Max ���ϱ�
			Max = TAG[ThreadIndex][TagIndex]._Max[0];
			for (int i = 0; i < 3; ++i)
			{
				if (TAG[ThreadIndex][TagIndex]._Max[i] < Max)
				{
					Max = (float)TAG[ThreadIndex][TagIndex]._Max[i] / 10;		// ����ũ�� ����
				}
			}

			AlignText_fwprintf_s(pFile, TAG[ThreadIndex][TagIndex]._TagName, Average, Min, Max, TAG[ThreadIndex][TagIndex]._Call);
		}
		fwprintf_s(pFile, L"----------------------------------------------------------------------------------------------------------------------------\n\n");
	}

	fclose(pFile);
}

void CTlsProfiling::ProfileReset()
{

}

void CTlsProfiling::AlignText_fwprintf_s(FILE* file, const WCHAR* tagname, float average, float min, float max, __int64 call)
{
	fwprintf_s(file, L"%-24s|", tagname);
	fwprintf_s(file, L"%22.4f%s|", average, L"\u00B5s");
	fwprintf_s(file, L"%22.4f%s|", min, L"\u00B5s");
	fwprintf_s(file, L"%22.4f%s|", max, L"\u00B5s");
	fwprintf_s(file, L"%24lld|\n", call);

	return;
}