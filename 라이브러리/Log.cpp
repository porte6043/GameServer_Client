#include "Log.h"

// ��Ÿ���� = EtcError
// �ý��� ���� = SYSTEM_INFO

CLog::CLog() : _LogCount(0), _LogLevel(LEVEL_DEBUG), _RootFile(L"Log")
{
    // �α� �⺻ Root File ����
    CreateDirectory(_RootFile, NULL);
}
CLog::~CLog() {}

CLog* CLog::GetInstance()
{
    static CLog Log;
    return &Log;
}

// ���� �ð� ����
void CLog::CurrTime(WCHAR* y, WCHAR* m, WCHAR* d, WCHAR* strTime)
{
    // ����ð� ���
    time_t now = time(nullptr);

    // ������ �´� �ð����� ��ȯ 
    tm localTime;
    localtime_s(&localTime, &now);

    // ��¥ �ʱ�ȭ
    //wcsftime(y, 9, L"%y-%m-%d", &localTime); // ������
    wcsftime(y, 3, L"%y", &localTime); // ������
    wcsftime(m, 3, L"%m", &localTime); // ������
    wcsftime(d, 3, L"%d", &localTime); // ������

    // �ð� �ʱ�ȭ
    wcsftime(strTime, 9, L"%H:%M:%S", &localTime); // ���� �ð�
}

// �α� ���� ��� ����
void CLog::LogRootFileSet(const WCHAR* fileName)
{
    WCHAR PrevRootFile[ROOTFILE_LEN] = { 0, };
    StringCchCopyW(PrevRootFile, ROOTFILE_LEN, _RootFile);

    LOG(L"SYSTEM_INFO", LEVEL_SYSTEM, L"Try Change RootFile [%s -> %s]", _RootFile, fileName);
    HRESULT Ret = StringCchCopyW(_RootFile, ROOTFILE_LEN, fileName);
    if (FAILED(Ret))
        LOG(L"EtcError", LEVEL_ERROR, L"Lack RootFile Name Buffer");

    LOG(L"SYSTEM_INFO", LEVEL_SYSTEM, L"Changed RootFile [%s -> %s]", PrevRootFile, fileName);

    return;
}

// �α� ���� ����
void CLog::LogLevelSet(long logLevel)
{
    InterlockedExchange(&_LogLevel, logLevel);
    LOG(L"SYSTEM_INFO", LEVEL_SYSTEM, L"Change LogLevel");
    return;
}

// Logging
void CLog::Log(const WCHAR* logCategory, int logLevel, const WCHAR* stringFormat, ...)
{
    // _LogLevel���� ũ�ų� ���� ���� Logging
    if (_LogLevel > logLevel)
        return;

    const WCHAR* Level[3] = { L"DEBUG", L"ERROR", L"SYSTEM" };
    WCHAR Time[9] = { 0, };
    WCHAR Y[3] = { 0, };
    WCHAR M[3] = { 0, };
    WCHAR D[3] = { 0, };
    WCHAR Path[PATH_LEN] = { 0, };
    WCHAR Message[MESSAGE_LEN] = { 0, };

    // ���� ��¥�� �ð� �ʱ�ȭ
    CurrTime(Y, M, D, Time);

    // txt��� �ʱ�ȭ
    HRESULT Path_Ret = StringCchPrintfW(Path, PATH_LEN, L"./%s/20%s/%s��/%s/20%s%s_%s.txt", _RootFile, Y, M, logCategory, Y, M, logCategory);
    if (FAILED(Path_Ret))
    {
        LOG(L"EtcError", LEVEL_ERROR, L"Lack LogPath Buffer");
        return;
    }

    // String Formet 
    va_list va;
    va_start(va, stringFormat);
    HRESULT Formet_Ret = StringCchVPrintfW(Message, MESSAGE_LEN, stringFormat, va);
    if (FAILED(Formet_Ret))
    {
        LOG(L"EtcError", LEVEL_ERROR, L"Lack Message Buffer");
    }
    va_end(va);


    // TXT ����
    FILE* pFile;
    if (_wfopen_s(&pFile, Path, L"ab") != 0)
    {
        // ���� ���� ���� ���� �غ�
        CreateFolder(Path);

        // ���� �� ���� ��� �õ�
        while (_wfopen_s(&pFile, Path, L"ab")) {} 
    }

    InterlockedIncrement(&_LogCount);
    fwprintf(pFile, L"[%s] [20%s-%s-%s %s / %s / %010u] %s\n",
        logCategory, Y, M, D, Time, Level[logLevel], _LogCount, Message);

    fclose(pFile);

    return;
}

void CLog::CreateFolder(WCHAR* path)
{
    WCHAR TempPath[PATH_LEN] = { 0, };

    // ���� ��ο��� ������ ���丮 �̸� ����
    WCHAR* LastSlash = wcsrchr(path, L'/');
    WCHAR StrTemp;

    if (LastSlash != nullptr) {

        // ������ ���丮 �̸��� �����Ͽ� ���丮 ��θ� ����
        StrTemp = *(LastSlash + 1);
        *(LastSlash + 1) = L'\0';

        int strlen = 0;
        for (int iCnt = 0; iCnt < PATH_LEN; ++iCnt)
        {
            if (path[iCnt] == '/')
            {
                for (int jCnt = iCnt + 1; jCnt < PATH_LEN - iCnt; ++jCnt)
                {
                    if (path[jCnt] == '/')
                    {
                        strlen = jCnt;
                        break;
                    }
                }

                memcpy(TempPath, path, sizeof(WCHAR) * strlen);
                CreateDirectory(TempPath, nullptr);
            }
        }


        *(LastSlash + 1) = StrTemp;  // ���� ��η� ����
    }

    return;
}
