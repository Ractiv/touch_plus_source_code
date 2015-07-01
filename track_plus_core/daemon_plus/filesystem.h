#pragma once

#include <direct.h>
#include <atlstr.h>
#include <ShlObj.h>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <atlbase.h>
#include <atlconv.h>
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