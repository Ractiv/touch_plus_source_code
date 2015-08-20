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

#pragma once

#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#include <Psapi.h>
#pragma comment(lib, "psapi.lib")
#elif __APPLE__
#include "string_functions.h"
#include "filesystem.h"
#endif

#include <iostream>
#include <string>
#include <thread>
#include "globals.h"

using namespace std;

int process_running(string name);
void create_process(string path, string name, bool show_window = true);
void kill_process(string name);