#ifndef __PERFOEMANCE_DATA_HELP__
#define __PERFOEMANCE_DATA_HELP__
#include <Windows.h>
#include <strsafe.h>
#include <string>
#include <Pdh.h>
#pragma comment(lib,"Pdh.lib")

struct PerformanceHardwareData
{
	float CpuTotal;
	float ProcessCpuTotal;
	float ProcessCpuKernel;
	float ProcessCpuUser;
	unsigned long ProcessNonPagePool;
	unsigned long MemoryNonPagePool;
	unsigned long ProcessUsingMemory;
	unsigned long AvailableMemory;
	double NetworkRecvByte;
	double NetworkSendByte;

	PerformanceHardwareData()
	{
		CpuTotal = 0;
		ProcessCpuTotal = 0;
		ProcessCpuKernel = 0;
		ProcessCpuUser = 0;
		ProcessNonPagePool = 0;
		MemoryNonPagePool = 0;
		ProcessUsingMemory = 0;
		AvailableMemory = 0;
		NetworkRecvByte = 0;
		NetworkSendByte = 0;
	}
};

class CPerformanceDataHelp
{
private:
	// --------------------------------------------------------------
	// 이더넷 하나에 대한 Send,Recv PDH 쿼리 정보.
	//--------------------------------------------------------------
	struct st_ETHERNET
	{
		bool _bUse;
		WCHAR _szName[128];
		PDH_HCOUNTER _pdh_Counter_Network_RecvBytes;
		PDH_HCOUNTER _pdh_Counter_Network_SendBytes;
	};
	st_ETHERNET _EthernetStruct[8]; // 랜카드 별 PDH 정보

private:
	PerformanceHardwareData Pdh;

	// PDH 쿼리 핸들 생성 (하나로 모든 데이터 수집 가능)
	PDH_HQUERY Query;
	//// PDH 리소스 카운터 생성 (여러개 수집시 이를 여러개 생성)
	PDH_HCOUNTER CpuTotal;
	PDH_HCOUNTER ProcessCpuTotal;
	PDH_HCOUNTER ProcessCpuKernel;
	PDH_HCOUNTER ProcessCpuUser;
	PDH_HCOUNTER ProcessNonPagePool;
	PDH_HCOUNTER MemoryNonPagePool;
	PDH_HCOUNTER ProcessUsingMemory;
	PDH_HCOUNTER AvailableMemory;

	WCHAR ProcessCpuTotalQuery[256];
	WCHAR ProcessCpuKernelQuery[256];
	WCHAR ProcessCpuUserQuery[256];
	WCHAR ProcessNonPagePoolQuery[256];
	WCHAR ProcessUsingMemoryQuery[256];

public: CPerformanceDataHelp();
public: ~CPerformanceDataHelp();

public: void monitor();
private: void GetSendRecvByte(PerformanceHardwareData& phd);
public: void GetPHD(PerformanceHardwareData& phd);

};
#endif