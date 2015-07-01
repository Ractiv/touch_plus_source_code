#pragma once

#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <Psapi.h>
#pragma comment(lib, "psapi.lib")

using namespace std;

int process_running(const string name);
void create_process(const string path, const string name, bool show_window = true, bool use_native_working_directory = false);
void kill_process(const string name);