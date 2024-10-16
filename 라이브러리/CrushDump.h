#ifndef __LIB_CRUSH_DUMP__
#define __LIB_CRUSH_DUMP__

#include <iostream>
#include <Windows.h>
#include <cstdlib>		// For _invalid_parameter_handler
#include <crtdbg.h>		// For _CrtSetReportMode
#include <DbgHelp.h>	// For _MINIDUMP_EXCEPTION_INFORMATION, MiniDumpWriteDump 
#pragma comment(lib, "Dbghelp.lib")

// Windows.h ���� ���� ��� ���� �߻�
#include <Psapi.h>		// For 	PROCESS_MEMORY_COUNTERS, GetProcessMemoryInfo
#undef PSAPI_VERSION
#define PSAPI_VERSION 1
#pragma comment(lib, "psapi.lib")

class CCrushDump
{
public:
	CCrushDump()
	{
		//_DumpCount = 0;

		_invalid_parameter_handler OldHandler, NewHandler;

		NewHandler = myInvalidParameterHandler;
		OldHandler = _set_invalid_parameter_handler(NewHandler);	// crt �Լ��� null ������ ���� �־��� ��..
		_CrtSetReportMode(_CRT_WARN, 0);		// CRT ���� �޽��� ǥ�� �ߴ�, �ٷ� ������ ������.
		_CrtSetReportMode(_CRT_ASSERT, 0);		// CRT ���� �޽��� ǥ�� �ߴ�, �ٷ� ������ ������.
		_CrtSetReportMode(_CRT_ERROR, 0);		// CRT ���� �޽��� ǥ�� �ߴ�, �ٷ� ������ ������.

		_CrtSetReportHook(_custom_Report_hook);

		//----------------------------------------------------------------------------
		// pure virtual function called ���� �ڵ鷯�� ����� ���� �Լ��� ��ȸ��Ų��.
		//----------------------------------------------------------------------------
		_set_purecall_handler(myPurecallHandler);

		SetHandlerDump();
	}

	static void Crush()
	{
		int* p = nullptr;
		*p = 0;
	}

	static LONG WINAPI MyExceptionFilter(__in PEXCEPTION_POINTERS pExceptionPointer)
	{
		static long _DumpCount = 0;

		int iWorkingMemory = 0;
		SYSTEMTIME stNowTime;

		//----------------------------------------------------------------------------
		// ���� �����忡�� ���ÿ� ������ ���� �� ������ �̸��� ������ ����� �̹� Write ������
		// ������ �ֱ� ������ ����ó���� ������. �׷��� ù ��° ������ ���� ���� ���μ����� ����ȴ�.
		// �׷��� ������ InterlockedIncrement�� �̿��Ͽ� �����̸��� �ٸ��� �Ѵ�.
		//----------------------------------------------------------------------------
		long DumpCount = InterlockedIncrement(&_DumpCount);	

		//----------------------------------------------------------------------------
		// ���� ���μ����� �޸� ��뷮�� ���´�
		//----------------------------------------------------------------------------
		HANDLE hProcess = 0;
		PROCESS_MEMORY_COUNTERS pmc;
		
		hProcess = GetCurrentProcess();
		if (hProcess == NULL)
			return 0;

		if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
		{
			iWorkingMemory = (int)(pmc.WorkingSetSize / 1024 / 1024);
		}
		CloseHandle(hProcess);

		//----------------------------------------------------------------------------
		// ���� ��¥�� �ð��� �˾ƿ´�.
		//----------------------------------------------------------------------------
		WCHAR FileName[MAX_PATH];
		GetLocalTime(&stNowTime);
		wsprintf(FileName, L"Dump_%d%02d%02d_%02d.%02d.%02d_%d_%dMB.dmp",
			stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay, stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond, DumpCount, iWorkingMemory);

		wprintf(L"\n\n\n!!! Crash Error!!! %d.%d.%d / %d:%d:%d \n",
			stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay, stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond);
		//wprintf(L"Now Save Dump File...\n");

		HANDLE hDumpFile = ::CreateFile(FileName,
			GENERIC_WRITE,
			FILE_SHARE_WRITE,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

		if (hDumpFile != INVALID_HANDLE_VALUE)
		{
			_MINIDUMP_EXCEPTION_INFORMATION MinidumpExceptionInformation;
			MinidumpExceptionInformation.ThreadId = ::GetCurrentThreadId();
			MinidumpExceptionInformation.ExceptionPointers = pExceptionPointer;
			MinidumpExceptionInformation.ClientPointers = TRUE;

			MiniDumpWriteDump(GetCurrentProcess(),
				GetCurrentProcessId(),
				hDumpFile,
				MiniDumpWithFullMemory,
				&MinidumpExceptionInformation,
				NULL,
				NULL);
			CloseHandle(hDumpFile);
			  
			wprintf(L"CrashDump Save Finish!!!");
		}

		return EXCEPTION_EXECUTE_HANDLER;
	}

	static void SetHandlerDump()
	{
		SetUnhandledExceptionFilter(MyExceptionFilter);
	}

	// Invaild Parameter handler
	static void myInvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t pReserved)
	{
		Crush();
	}

	static int _custom_Report_hook(int ireposttype, char* message, int* returnvalue)
	{
		Crush();
		return true;
	}

	static void myPurecallHandler(void)
	{
		Crush();
	}

	//static long _DumpCount;
};

//long CCrushDump::_DumpCount = 0;
#endif
