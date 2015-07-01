#pragma once

#ifdef _WIN32
#include <direct.h>
#include <atlstr.h>
#include <ShlObj.h>
#include <windows.h>
#include <iostream>
#include <fstream>
#include "dirent.h"
#endif
#include <vector>
#include <string>

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