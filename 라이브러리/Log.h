#include <iostream>
#include <ctime>
#include <strsafe.h>
#include <Windows.h>

#ifndef __LOG__
#define __LOG__

#define LOG(category, level, stringFormat, ...)                                 \
do{                                                                             \
    CLog::GetInstance()->Log(category, level, stringFormat, __VA_ARGS__);       \
}while(0)                                                             

// �α��� Root File ���� (�⺻ Log ���Ͽ� ����) 
#define LOG_DIRECTORY(FILENAME)                                      \
do{                                                                  \
    CLog::GetInstance()->LogRootFileSet(FILENAME);                   \
}while(0)                                                             

// �α��� Log Level ���� 
#define LOG_LEVEL(LOGLEVEL)                                     \
do{                                                             \
    CLog::GetInstance()->LogLevelSet(LOGLEVEL);                 \
}while(0)                           



class CLog
{
public:
    // SingleTon �α� Ŭ����
    static CLog* GetInstance();

    enum LOG_LEVER
    {
        LEVEL_DEBUG,
        LEVEL_ERROR,
        LEVEL_SYSTEM
    };

private:
    enum LOG_VALUE
    {
        PATH_LEN = 256,
        MESSAGE_LEN = 256,
        ROOTFILE_LEN = 128
    };

private:
    DWORD _LogCount;
    long _LogLevel;
    WCHAR _RootFile[ROOTFILE_LEN];

private:
    CLog();
    ~CLog();


    // �ý��� �α�
public:
    // �α� ���� ��� ����
    void LogRootFileSet(const WCHAR* logRootFileName);

    // �α� ���� ����
    void LogLevelSet(long logLevel);

    // Logging
    void Log(const WCHAR* logCategory, int logLevel, const WCHAR* stringFormat, ...);

    // �ý��� �α�
private:
    void CreateFolder(WCHAR* path);

    // strdate len = 9 , strtime len = 9
    void CurrTime(WCHAR* y, WCHAR* m, WCHAR* d, WCHAR* strTime);
};



#endif