#include "PerformanceDataHelp.h"


CPerformanceDataHelp::CPerformanceDataHelp() : 
	ProcessCpuTotalQuery{}, ProcessCpuKernelQuery{},
	ProcessCpuUserQuery{}, ProcessNonPagePoolQuery{},
	ProcessUsingMemoryQuery{}
{
	WCHAR buf[256] = { 0, };
	DWORD bufLen = sizeof(buf);
	DWORD pid = GetCurrentProcessId();
	HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);	// 프로세스 pid로 프로세스 핸들 검색
	QueryFullProcessImageName(hProc, 0, buf, &bufLen);							// 프로세스 핸들을 통해 전체 경로 찾기
	CloseHandle(hProc);

	// 파일 경로에서 디렉토리 구분자를 찾음
	std::wstring ProcessPath = buf;
	size_t StartIndex = ProcessPath.find_last_of(L"\\/");
	size_t LastIndex = ProcessPath.find_last_of(L".");

	// 프로세스 이름
	std::wstring ProcessName = ProcessPath.substr(StartIndex + 1, LastIndex - StartIndex - 1);

	// 쿼리문 입력
	StringCchPrintfW(ProcessCpuTotalQuery, 256, L"\\Process(%s)\\%% Processor Time", ProcessName.c_str());
	StringCchPrintfW(ProcessCpuKernelQuery, 256, L"\\Process(%s)\\%% Privileged Time", ProcessName.c_str());
	StringCchPrintfW(ProcessCpuUserQuery, 256, L"\\Process(%s)\\%% User Time", ProcessName.c_str());
	StringCchPrintfW(ProcessNonPagePoolQuery, 256, L"\\Process(%s)\\Pool Nonpaged Bytes", ProcessName.c_str());
	StringCchPrintfW(ProcessUsingMemoryQuery, 256, L"\\Process(%s)\\Private Bytes", ProcessName.c_str());
	
	//	\Memory\Pool Nonpaged Bytes		// processor Nonpaged 
	 
	
	// PDH 쿼리 핸들 생성 (하나로 모든 데이터 수집 가능)
	PdhOpenQuery(NULL, NULL, &Query);

	//// PDH 리소스 카운터 생성 (여러개 수집시 이를 여러개 생성)
	PdhAddCounter(Query, L"\\Processor(_Total)\\% Processor Time", NULL, &CpuTotal);
	PdhAddCounter(Query, ProcessCpuTotalQuery, NULL, &ProcessCpuTotal);
	PdhAddCounter(Query, ProcessCpuKernelQuery, NULL, &ProcessCpuKernel);
	PdhAddCounter(Query, ProcessCpuUserQuery, NULL, &ProcessCpuUser);
	PdhAddCounter(Query, ProcessNonPagePoolQuery, NULL, &ProcessNonPagePool);
	PdhAddCounter(Query, L"\\Memory\\Pool Nonpaged Bytes", NULL, &MemoryNonPagePool);
	PdhAddCounter(Query, ProcessUsingMemoryQuery, NULL, &ProcessUsingMemory);
	PdhAddCounter(Query, L"\\Memory\\Available MBytes", NULL, &AvailableMemory);

	// Send Recv 쿼리문
	int iCnt = 0;
	bool bErr = false;
	WCHAR* szCur = NULL;
	WCHAR* szCounters = NULL;
	WCHAR* szInterfaces = NULL;
	DWORD dwCounterSize = 0, dwInterfaceSize = 0;
	WCHAR szQuery[1024] = { 0, };

	// PDH enum Object 를 사용하는 방법.
	// 모든 이더넷 이름이 나오지만 실제 사용중인 이더넷, 가상이더넷 등등을 확인불가 함.
	//---------------------------------------------------------------------------------------
	// PdhEnumObjectItems 을 통해서 "NetworkInterface" 항목에서 얻을 수 있는
	// 측성항목(Counters) / 인터페이스 항목(Interfaces) 를 얻음. 그런데 그 개수나 길이를 모르기 때문에
	// 먼저 버퍼의 길이를 알기 위해서 Out Buffer 인자들을 NULL 포인터로 넣어서 사이즈만 확인.
	//---------------------------------------------------------------------------------------
	PdhEnumObjectItems(NULL, NULL, L"Network Interface", szCounters, &dwCounterSize, szInterfaces, &dwInterfaceSize, PERF_DETAIL_WIZARD, 0);
	szCounters = new WCHAR[dwCounterSize];
	szInterfaces = new WCHAR[dwInterfaceSize];
	//---------------------------------------------------------------------------------------
	// 버퍼의 동적할당 후 다시 호출!
	// 
	// szCounters 와 szInterfaces 버퍼에는 여러개의 문자열이 쭉쭉쭉 들어온다. 2차원 배열도 아니고,
	// 그냥 NULL 포인터로 끝나는 문자열들이 dwCounterSize, dwInterfaceSize 길이만큼 줄줄이 들어있음.
	// 이를 문자열 단위로 끊어서 개수를 확인 해야 함. aaa\0bbb\0ccc\0ddd 이딴 식
	//---------------------------------------------------------------------------------------
	if (PdhEnumObjectItems(NULL, NULL, L"Network Interface", szCounters, &dwCounterSize, szInterfaces, &dwInterfaceSize, PERF_DETAIL_WIZARD,
		0) != ERROR_SUCCESS)
	{
		delete[] szCounters;
		delete[] szInterfaces;
		return;
	}
	iCnt = 0;
	szCur = szInterfaces;
	//---------------------------------------------------------
	// szInterfaces 에서 문자열 단위로 끊으면서 , 이름을 복사받는다.
	//---------------------------------------------------------
	for (; *szCur != L'\0' && iCnt < 8; szCur += wcslen(szCur) + 1, iCnt++)
	{
		_EthernetStruct[iCnt]._bUse = true;
		_EthernetStruct[iCnt]._szName[0] = L'\0';
		wcscpy_s(_EthernetStruct[iCnt]._szName, szCur);
		szQuery[0] = L'\0';
		StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Network Interface(%s)\\Bytes Received/sec", szCur);
		PdhAddCounter(Query, szQuery, NULL, &_EthernetStruct[iCnt]._pdh_Counter_Network_RecvBytes);
		szQuery[0] = L'\0';
		StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Network Interface(%s)\\Bytes Sent/sec", szCur);
		PdhAddCounter(Query, szQuery, NULL, &_EthernetStruct[iCnt]._pdh_Counter_Network_SendBytes);
	}
}
CPerformanceDataHelp::~CPerformanceDataHelp()
{

}
void CPerformanceDataHelp::monitor()
{
	//// 데이터 갱신
	//PdhCollectQueryData(Query);
	//SYSTEM_INFO si;
	//GetSystemInfo(&si);

	//PDH_FMT_COUNTERVALUE CountValue;
	///*PdhGetFormattedCounterValue(CpuTotal, PDH_FMT_DOUBLE, NULL, &CountValue);
	//wprintf(L"CPU : %f%% ", CountValue.doubleValue);
	//PdhGetFormattedCounterValue(ProcessCpuTotal, PDH_FMT_DOUBLE, NULL, &CountValue);
	//wprintf(L"[Chatting Total:%.0f%% ", CountValue.doubleValue / si.dwNumberOfProcessors);
	//PdhGetFormattedCounterValue(ProcessCpuKernel, PDH_FMT_DOUBLE, NULL, &CountValue);
	//wprintf(L"K:%.0f%% ", CountValue.doubleValue / si.dwNumberOfProcessors);
	//PdhGetFormattedCounterValue(ProcessCpuUser, PDH_FMT_DOUBLE, NULL, &CountValue);
	//wprintf(L"U:%.0f%%]\n", CountValue.doubleValue / si.dwNumberOfProcessors);*/
	//PdhGetFormattedCounterValue(ProcessNonPagePool, PDH_FMT_LONG, NULL, &CountValue);
	//wprintf(L"NonPagePool : %dKByte\n", (long)(CountValue.longValue / 1024));
	//PdhGetFormattedCounterValue(ProcessUsingMemory, PDH_FMT_LONG, NULL, &CountValue);
	//wprintf(L"UsingMemory : %dMByte\n", (long)(CountValue.longValue / 1024 / 1024));
	////PdhGetFormattedCounterValue(AvailableMemory, PDH_FMT_LONG, NULL, &CountValue);
	////wprintf(L"AvailableMemory		: %dMByte\n", CountValue.longValue);

	//return;
}
void CPerformanceDataHelp::GetSendRecvByte(PerformanceHardwareData& phd)
{
	//-----------------------------------------------------------------------------------------------
	// 이더넷 개수만큼 돌면서 총 합을 뽑음.
	//-----------------------------------------------------------------------------------------------
	PDH_FMT_COUNTERVALUE CountValue;
	double RecvSum = 0;
	double SendSum = 0;
	for (int iCnt = 0; iCnt < 8; iCnt++)
	{
		if (_EthernetStruct[iCnt]._bUse)
		{
			
			PdhGetFormattedCounterValue(_EthernetStruct[iCnt]._pdh_Counter_Network_RecvBytes,PDH_FMT_DOUBLE, NULL, &CountValue);
			RecvSum += CountValue.doubleValue;

			
			PdhGetFormattedCounterValue(_EthernetStruct[iCnt]._pdh_Counter_Network_SendBytes, PDH_FMT_DOUBLE, NULL, &CountValue);
			SendSum += CountValue.doubleValue;
		}
	}
	phd.NetworkRecvByte = RecvSum;
	phd.NetworkSendByte = SendSum;

	return;
}
void CPerformanceDataHelp::GetPHD(PerformanceHardwareData& phd)
{
	PdhCollectQueryData(Query);

	PDH_FMT_COUNTERVALUE CountValue;
	PdhGetFormattedCounterValue(CpuTotal, PDH_FMT_DOUBLE, NULL, &CountValue);
	phd.CpuTotal = CountValue.doubleValue;

	PdhGetFormattedCounterValue(ProcessCpuTotal, PDH_FMT_DOUBLE, NULL, &CountValue);
	phd.ProcessCpuTotal = CountValue.doubleValue;

	PdhGetFormattedCounterValue(ProcessCpuKernel, PDH_FMT_DOUBLE, NULL, &CountValue);
	phd.ProcessCpuKernel = CountValue.doubleValue;

	PdhGetFormattedCounterValue(ProcessCpuUser, PDH_FMT_DOUBLE, NULL, &CountValue);
	phd.ProcessCpuUser = CountValue.doubleValue;

	PdhGetFormattedCounterValue(ProcessNonPagePool, PDH_FMT_LONG, NULL, &CountValue);
	phd.ProcessNonPagePool = CountValue.longValue;

	PdhGetFormattedCounterValue(MemoryNonPagePool, PDH_FMT_LONG, NULL, &CountValue);
	phd.MemoryNonPagePool = CountValue.longValue;

	PdhGetFormattedCounterValue(ProcessUsingMemory, PDH_FMT_LONG, NULL, &CountValue);
	phd.ProcessUsingMemory = CountValue.longValue;

	PdhGetFormattedCounterValue(AvailableMemory, PDH_FMT_LONG, NULL, &CountValue);
	phd.AvailableMemory = CountValue.longValue;

	GetSendRecvByte(phd);

	return;
}