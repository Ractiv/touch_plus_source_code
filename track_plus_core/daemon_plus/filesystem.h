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
#include <direct.h>
#include <atlstr.h>
#include <ShlObj.h>
#include <windows.h>
#include <atlbase.h>
#include <atlconv.h>
#elif __APPLE__
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#endif

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include "dirent.h"
#include "string_functions.h"
#include "globals.h"

using namespace std;

bool directory_exists(const string path);
bool file_exists(const string path);
vector<string> list_files_in_directory(const string path);
void create_directory(const string path);
void write_string_to_file(const string path, const string str);
vector<string> read_text_file(const string path);
void copy_file(const string src_path, const string dst_path);
void delete_file(const string path);
void delete_all_files(const string path);
void rename_file(const string path_old, const string path_new);
string get_startup_folder_path();
int create_shortcut(string src_path, string dst_path, string working_directory);