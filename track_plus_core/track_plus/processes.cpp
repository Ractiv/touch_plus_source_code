#include "processes.h"

int process_running(const string name)
{
    int process_count = 0;
    HANDLE SnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if(SnapShot == INVALID_HANDLE_VALUE)
        return false;

    PROCESSENTRY32 procEntry;
    procEntry.dwSize = sizeof(PROCESSENTRY32);

    if(!Process32First(SnapShot, &procEntry))
        return false;

    do
    {
        if(strcmp(procEntry.szExeFile, name.c_str()) == 0)
            ++process_count;
    }
    while (Process32Next(SnapShot, &procEntry));

    return process_count;
}

void create_process(const string path, const string name, bool show_window, bool use_native_working_directory)
{
    if (!use_native_working_directory)
    {
        PROCESS_INFORMATION ProcessInfo;
        STARTUPINFO StartupInfo;
        ZeroMemory(&StartupInfo, sizeof(StartupInfo));
        StartupInfo.cb = sizeof(StartupInfo);

        if (show_window)
            CreateProcess(path.c_str(), NULL, NULL, NULL, FALSE, REALTIME_PRIORITY_CLASS, NULL, NULL, &StartupInfo, &ProcessInfo);
        else
            CreateProcess(path.c_str(), NULL, NULL, NULL, FALSE,
                          REALTIME_PRIORITY_CLASS | CREATE_NO_WINDOW, NULL, NULL, &StartupInfo, &ProcessInfo);

        CloseHandle(ProcessInfo.hThread);
        CloseHandle(ProcessInfo.hProcess);
    }
    else
        ShellExecute(NULL, "runas", path.c_str(), NULL, NULL, SW_SHOWNORMAL);

    while (process_running(name.c_str()) == 0)
        Sleep(1);
}

void kill_process(const string name)
{
    CHAR szProcBuff[101];
    DWORD pIDs[300], dwBytesReturned;
    HANDLE hProcess;
    INT i, procCount;

    EnumProcesses(pIDs, sizeof(pIDs), &dwBytesReturned);
    procCount = dwBytesReturned / sizeof(DWORD);

    for (i = 0; i < procCount; i++)
        if (pIDs[i] != 0)
        {
            hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_TERMINATE, 0, pIDs[i]);
            GetModuleBaseName(hProcess, 0, szProcBuff, 100);

            if (strcmp(szProcBuff, name.c_str()) == 0)
                TerminateProcess(hProcess, EXIT_SUCCESS);

            CloseHandle(hProcess);
        }
}