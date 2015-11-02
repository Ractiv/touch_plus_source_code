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

bool process_running(string name)
{
    while (processes_temp_path == "")
        Sleep(1000);

    while (true)
    {
        bool locked = false;
        vector<string> file_name_vec = list_files_in_directory(processes_temp_path);
        for (string file_name_current : file_name_vec)
            if (file_name_current.find("lock") != std::string::npos)
            {
                locked = true;
                break;
            }

        if (!locked)
            break;

        Sleep(1000);
    }

    write_string_to_file(processes_temp_path + "/lock_" + module_name, "");

#ifdef _WIN32
    string command = "tasklist > " + processes_temp_path + "/processes_" + module_name;
#elif __APPLE__
    string command = "ps -e > " + processes_temp_path + "/processes_" + module_name;
#endif

    system(command.c_str());
    vector<string> lines = read_text_file(processes_temp_path + "/processes_" + module_name);

    delete_file(processes_temp_path + "/processes_" + module_name);

    for (string& str : lines)
        if (str.find(name) != std::string::npos)
        {
            delete_file(processes_temp_path + "/lock_" + module_name);
            return true;
        }

    delete_file(processes_temp_path + "/lock_" + module_name);
    return false;
}

void create_process(string path, string name, bool is_app)
{
#ifdef _WIN32
    string command = "start " + path;

#elif __APPLE__
    string command;
    if (!is_app)
        command = path + " &";
    else
        command = "open " + path;

#endif

    system(command.c_str());

    while (!process_running(name))
        Sleep(1000);
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

    while (process_running(name))
        Sleep(1000);
}