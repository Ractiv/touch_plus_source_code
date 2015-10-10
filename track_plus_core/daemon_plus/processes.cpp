/*
 * Touch+ Software
 * Copyright (C) 2015
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the Aladdin Free Public License as
 * published by the Aladdin Enterprises, either version 9 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * Aladdin Free Public License for more details.
 *
 * You should have received a copy of the Aladdin Free Public License
 * along with this program.  If not, see <http://ghostscript.com/doc/8.54/Public.htm>.
 */

#include "processes.h"
#include "console_log.h"

int check_process_running(string name, bool partial)
{
#ifdef _WIN32
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

#elif __APPLE__
    vector<string> file_name_vec = list_files_in_directory(executable_path);
    for (string file_name_current : file_name_vec)
        if (file_name_current == "lock")
            return true;

    write_string_to_file(executable_path + "/lock", "");

    string command = "ps -e > " + executable_path + "/processes.txt";
    system(command.c_str());
    
    vector<string> lines = read_text_file(executable_path + "/processes.txt");
    delete_file(executable_path + "/processes.txt");

    for (string& str : lines)
        if (partial)
        {
            if (str.find(name) != std::string::npos)
            {
                delete_file(executable_path + "/lock");
                return true;
            }
        }
        else
        {
            vector<string> str_parts = split_string(str, "/");
            if (str_parts.size() > 1)
            {
                string process_name = str_parts[str_parts.size() - 1];
                if (process_name == name)
                {
                    delete_file(executable_path + "/lock");
                    return true;
                }
            }
        }
    delete_file(executable_path + "/lock");
    return false;
#endif
}

int process_running(string name, bool partial)
{
    int result = check_process_running(name, partial);
    
    if (result == 0)
        console_log("process not running " + name);

    return result;
}

void create_process(string path, string name, bool show_window, bool is_app, bool partial)
{ 
#ifdef _WIN32
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

#elif __APPLE__
    string command;
    if (!is_app)
        command = path + " &";
    else
        command = "open " + path;

    console_log(command);

    system(command.c_str());
#endif

    while (process_running(name.c_str(), partial) == 0)
        Sleep(100);
}

void kill_process(string name)
{
#ifdef _WIN32
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

#elif __APPLE__
        string command = "killall -kill " + name;
        system(command.c_str());
#endif

    while (process_running(name.c_str()) > 0)
        Sleep(1);
}