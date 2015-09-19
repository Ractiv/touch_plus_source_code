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

#include "filesystem.h"

bool directory_exists(const string path)
{
#ifdef _WIN32
	DWORD ftyp = GetFileAttributesA(path.c_str());
	if (ftyp == INVALID_FILE_ATTRIBUTES)
		return false;

	if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
		return true;

#elif __APPLE__
	struct stat info;

	if (stat(path.c_str(), &info) != 0)
		return false;
	else if (info.st_mode & S_IFDIR)
		return true;
#endif

	return false;
}

bool file_exists(const string path)
{
	ifstream f(path.c_str());
	if (f.good())
	{
		f.close();
		return true;
	}
	else
	{
		f.close();
		return false;
	}
}

vector<string> list_files_in_directory(const string path)
{
	vector<string> file_name_vec = vector<string>();

	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir(path.c_str())) != NULL)
	{
		while ((ent = readdir(dir)) != NULL)
			if (ent->d_name[0] != 46)
				file_name_vec.push_back(ent->d_name);

		closedir(dir);
	}
	return file_name_vec;
}

void create_directory(const string path)
{
#ifdef _WIN32
	CreateDirectory(path.c_str(), NULL);

#elif __APPLE__
	mkdir(path.c_str(), 0775);
#endif
}

void write_string_to_file(const string path, const string str)
{
	ofstream out(path);
	out << str.c_str();
	out.close();
}

vector<string> read_text_file(const string path)
{
	vector<string> lines = vector<string>();
	string line;
	ifstream in_file(path);
	if (in_file)
		while (getline(in_file, line))
			lines.push_back(line);

	in_file.close();
	return lines;
}

void copy_file(const string src_path, const string dst_path)
{
	ifstream src(src_path, ios::binary);
	ofstream dest(dst_path, ios::binary);
	dest << src.rdbuf();
}

void delete_file(const string path)
{
	while (remove(path.c_str()) != 0)
		Sleep(1);
}

void delete_all_files(const string path)
{
	vector<string> file_name_vec = list_files_in_directory(path);
	for (string file_name : file_name_vec)
		delete_file(path + slash + file_name);
}

void rename_file(const string path_old, const string path_new)
{
	rename(path_old.c_str(), path_new.c_str());
}

#ifdef _WIN32
string get_startup_folder_path()
{
	PWSTR pszPath;
	HRESULT hr = SHGetKnownFolderPath(FOLDERID_Startup, 0, NULL, &pszPath);

	if (SUCCEEDED(hr))
	{
		wstring path(pszPath);
		CoTaskMemFree(static_cast<LPVOID>(pszPath));

		return to_string(path);
	}
	else
		return 0;
}
#endif

int create_shortcut(string src_path, string dst_path, string working_directory)
{
#ifdef _WIN32
	CoInitialize(NULL);
	IShellLink* pShellLink = NULL;
	HRESULT hres;
	hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_ALL,
		IID_IShellLink, (void**)&pShellLink);
	COUT << hex << hres << endl;
	if (SUCCEEDED(hres))
	{
		pShellLink->SetPath(src_path.c_str());
		pShellLink->SetDescription("");
		pShellLink->SetIconLocation(src_path.c_str(), 0);
		pShellLink->SetWorkingDirectory(working_directory.c_str());

		IPersistFile *pPersistFile;
		hres = pShellLink->QueryInterface(IID_IPersistFile, (void**)&pPersistFile);

		if (SUCCEEDED(hres))
		{
			hres = pPersistFile->Save(CA2W(dst_path.c_str()), TRUE);
			pPersistFile->Release();
		}
		else
		{
			COUT << "Error 2" << endl;
			return 2;
		}
		pShellLink->Release();
	}
	else
	{
		COUT << "Error 1" << endl;
		return 1;
	}
	
#elif __APPLE__
	//todo: port to OSX
#endif

	return 0;
}