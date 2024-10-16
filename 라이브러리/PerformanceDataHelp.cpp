#include "PerformanceDataHelp.h"


CPerformanceDataHelp::CPerformanceDataHelp() : 
	ProcessCpuTotalQuery{}, ProcessCpuKernelQuery{},
	ProcessCpuUserQuery{}, ProcessNonPagePoolQuery{},
	ProcessUsingMemoryQuery{}
{
	WCHAR buf[256] = { 0, };
	DWORD bufLen = sizeof(buf);
	DWORD pid = GetCurrentProcessId();
	HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);	// ���μ��� pid�� ���μ��� �ڵ� �˻�
	QueryFullProcessImageName(hProc, 0, buf, &bufLen);							// ���μ��� �ڵ��� ���� ��ü ��� ã��
	CloseHandle(hProc);

	// ���� ��ο��� ���丮 �����ڸ� ã��
	std::wstring ProcessPath = buf;
	size_t StartIndex = ProcessPath.find_last_of(L"\\/");
	size_t LastIndex = ProcessPath.find_last_of(L".");

	// ���μ��� �̸�
	std::wstring ProcessName = ProcessPath.substr(StartIndex + 1, LastIndex - StartIndex - 1);

	// ������ �Է�
	StringCchPrintfW(ProcessCpuTotalQuery, 256, L"\\Process(%s)\\%% Processor Time", ProcessName.c_str());
	StringCchPrintfW(ProcessCpuKernelQuery, 256, L"\\Process(%s)\\%% Privileged Time", ProcessName.c_str());
	StringCchPrintfW(ProcessCpuUserQuery, 256, L"\\Process(%s)\\%% User Time", ProcessName.c_str());
	StringCchPrintfW(ProcessNonPagePoolQuery, 256, L"\\Process(%s)\\Pool Nonpaged Bytes", ProcessName.c_str());
	StringCchPrintfW(ProcessUsingMemoryQuery, 256, L"\\Process(%s)\\Private Bytes", ProcessName.c_str());
	
	//	\Memory\Pool Nonpaged Bytes		// processor Nonpaged 
	 
	
	// PDH ���� �ڵ� ���� (�ϳ��� ��� ������ ���� ����)
	PdhOpenQuery(NULL, NULL, &Query);

	//// PDH ���ҽ� ī���� ���� (������ ������ �̸� ������ ����)
	PdhAddCounter(Query, L"\\Processor(_Total)\\% Processor Time", NULL, &CpuTotal);
	PdhAddCounter(Query, ProcessCpuTotalQuery, NULL, &ProcessCpuTotal);
	PdhAddCounter(Query, ProcessCpuKernelQuery, NULL, &ProcessCpuKernel);
	PdhAddCounter(Query, ProcessCpuUserQuery, NULL, &ProcessCpuUser);
	PdhAddCounter(Query, ProcessNonPagePoolQuery, NULL, &ProcessNonPagePool);
	PdhAddCounter(Query, L"\\Memory\\Pool Nonpaged Bytes", NULL, &MemoryNonPagePool);
	PdhAddCounter(Query, ProcessUsingMemoryQuery, NULL, &ProcessUsingMemory);
	PdhAddCounter(Query, L"\\Memory\\Available MBytes", NULL, &AvailableMemory);

	// Send Recv ������
	int iCnt = 0;
	bool bErr = false;
	WCHAR* szCur = NULL;
	WCHAR* szCounters = NULL;
	WCHAR* szInterfaces = NULL;
	DWORD dwCounterSize = 0, dwInterfaceSize = 0;
	WCHAR szQuery[1024] = { 0, };

	// PDH enum Object �� ����ϴ� ���.
	// ��� �̴��� �̸��� �������� ���� ������� �̴���, �����̴��� ����� Ȯ�κҰ� ��.
	//---------------------------------------------------------------------------------------
	// PdhEnumObjectItems �� ���ؼ� "NetworkInterface" �׸񿡼� ���� �� �ִ�
	// �����׸�(Counters) / �������̽� �׸�(Interfaces) �� ����. �׷��� �� ������ ���̸� �𸣱� ������
	// ���� ������ ���̸� �˱� ���ؼ� Out Buffer ���ڵ��� NULL �����ͷ� �־ ����� Ȯ��.
	//---------------------------------------------------------------------------------------
	PdhEnumObjectItems(NULL, NULL, L"Network Interface", szCounters, &dwCounterSize, szInterfaces, &dwInterfaceSize, PERF_DETAIL_WIZARD, 0);
	szCounters = new WCHAR[dwCounterSize];
	szInterfaces = new WCHAR[dwInterfaceSize];
	//---------------------------------------------------------------------------------------
	// ������ �����Ҵ� �� �ٽ� ȣ��!
	// 
	// szCounters �� szInterfaces ���ۿ��� �������� ���ڿ��� ������ ���´�. 2���� �迭�� �ƴϰ�,
	// �׳� NULL �����ͷ� ������ ���ڿ����� dwCounterSize, dwInterfaceSize ���̸�ŭ ������ �������.
	// �̸� ���ڿ� ������ ��� ������ Ȯ�� �ؾ� ��. aaa\0bbb\0ccc\0ddd �̵� ��
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
	// szInterfaces ���� ���ڿ� ������ �����鼭 , �̸��� ����޴´�.
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
	//// ������ ����
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
	// �̴��� ������ŭ ���鼭 �� ���� ����.
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