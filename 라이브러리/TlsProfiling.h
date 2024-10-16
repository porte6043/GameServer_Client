#include <stdio.h>
#include <iostream>
#include <windows.h>
#pragma comment(lib, "Winmm.lib")

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
// �� ó���� #define PROFILE �� �׻� ���ش�
// 
// PRO_BEGIN(L"tagname");
// profile�� �ڵ�..
// PRO_END(L"tagname");
// 
// ������ �Ҹ����� �̿��� Profile
// CTlsProfile Profile(L"tagname"); 
// 
// 
// txt���Ϸ� ��� �ϰ� ���� �� CTlsProfiling::GetInstance()->ProfileDataOutText(L"txt.�����̸�");
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-


// Ŭ���� ����
#ifndef __TLSPROFILING__
#define __TLSPROFILING__

#define PROFILE
#ifdef PROFILE
#define PRO_BEGIN(TagName) CTlsProfiling::GetInstance()->ProfileBegin(TagName)
#define PRO_END(TagName) CTlsProfiling::GetInstance()->ProfileEnd(TagName)
#else
#define PRO_BEGIN(TagName)
#define PRO_END(TagName)
#endif

#define TAGARRCOUNT 25
#define THREADCOUNT 10

class CTlsProfiling
{
private:
	struct TAG_NAME
	{
		bool _Flag;					// ���������� ��� ����(�迭 ��� ��)
		WCHAR _TagName[64];			// �������� Tag �̸�
		LARGE_INTEGER _StartTime;	// �������� ���� �ð�
		DWORD64 _TotalTime;			// ��ü ���ð� (��½� ȣ��ȸ��:Call�� ������ ����� ����)
		DWORD64	_Min[3];			// �ּ� ���� �ð� (0->1->2 ���� ����)
		DWORD64 _Max[3];			// �ִ� ���� �ð� (0->1->2 ū ����)
		DWORD64 _Call;				// ȣ�� Ƚ��
	};

private:
	TAG_NAME TAG[THREADCOUNT][TAGARRCOUNT];
	DWORD ThreadID[THREADCOUNT];
	DWORD TlsIndex;					// Tls �ε����� ��
	DWORD TagNameIndex;				// TAG_NAME �迭�� �ε��� ��


private: CTlsProfiling();
private: ~CTlsProfiling();

private: void ProfileReset();

private: void AlignText_fwprintf_s(FILE* file, const WCHAR* tagname, float average, float min, float max, __int64 call);

public: static CTlsProfiling* GetInstance();

public: void ProfileBegin(const WCHAR* tagname);

public: void ProfileEnd(const WCHAR* tagname);

public: void ProfileDataOutText(const WCHAR* TextFileName);
};

class CTlsProfile
{
friend class CTlsProfiling;  
private:
	const WCHAR* TagName;

public:
	CTlsProfile(const WCHAR* tagname)
	{ 
		TagName = tagname;
		PRO_BEGIN(TagName);
	}
	~CTlsProfile()
	{
		PRO_END(TagName);
	}
};

#endif  