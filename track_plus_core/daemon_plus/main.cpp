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

#include <thread>
#include "settings.h"
#include "globals.h"
#include "ipc.h"
#include "processes.h"

#ifdef SHOW_CONSOLE
#pragma comment(linker, "/subsystem:console")
#else
#pragma comment(linker, "/subsystem:windows")
#endif

using namespace std;

IPC ipc("daemon_plus");
IPC* ipc_ptr_global = &ipc;

bool block_guardian = false;

UDP udp;

void guardian_thread_function()
{
	while (true)
	{
		if (block_guardian)
		{
			Sleep(1000);
			continue;
		}

		if (process_running("track_plus.exe") == 0)
		{
			COUT << "track_plus created" << endl;

#ifdef SHOW_CONSOLE
			bool show_console = true;
#else
			bool show_console = false;
#endif
			create_process(executable_path + slash + "track_plus.exe", "track_plus.exe", show_console);
			ipc.send_message("menu_plus", "show stage", "");
		}

		if (process_running("menu_plus.exe") == 0)
		{
			COUT << "menu_plus created" << endl;
			create_process(executable_path + slash + "menu_plus" + slash + "menu_plus.exe", "menu_plus.exe");
		}

		Sleep(500);
	}
}

void ipc_thread_function()
{
	while (true)
	{
		ipc.update();
		Sleep(100);
	}
}

#ifdef SHOW_CONSOLE
int main()
#else
int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
#endif
{
#ifdef _WIN32
	char buffer[MAX_PATH];
    GetModuleFileName(NULL, buffer, MAX_PATH);
    string::size_type pos = string(buffer).find_last_of("\\/");
    executable_path = string(buffer).substr(0, pos);
#elif __APPLE__
    //todo: port to OSX
#endif
	data_path = executable_path + slash + "userdata";
	settings_file_path = data_path + slash + "settings.nrocinunerrad";
	ipc_path = executable_path + slash + "ipc";

	if (!directory_exists(ipc_path))
		create_directory(ipc_path);
	else
		delete_all_files(ipc_path);

	Settings settings;

	if (!file_exists(settings_file_path))
	{
		settings.launch_on_startup = "1";
		settings.power_saving_mode = "0";
		settings.check_for_updates = "1";
		settings.touch_control = "1";
		settings.table_mode = "0";
		settings.auto_detect_interaction_plane = "0";
		settings.scroll_bar_adjust_click_height_step = "5";

		if (!directory_exists(data_path))
			create_directory(data_path);
		
		ofstream settings_ofs(settings_file_path, ios::binary);
		settings_ofs.write((char*)&settings, sizeof(settings));

		COUT << "settings file created" << endl;
	}
	else
	{
		ifstream ifs(settings_file_path, ios::binary);
		ifs.read((char*)&settings, sizeof(settings));

		COUT << "settings file loaded" << endl;
	}

	IPC* ipc_ptr = &ipc;
	Settings* settings_ptr = &settings;
	ipc.map_function("get toggles", [ipc_ptr, settings_ptr](const string message_body)
	{
		string response = "";
		response += settings_ptr->launch_on_startup
				 +  settings_ptr->power_saving_mode
				 +  settings_ptr->check_for_updates
				 +  settings_ptr->touch_control
				 +  settings_ptr->table_mode
				 +  settings_ptr->auto_detect_interaction_plane
				 +  settings_ptr->scroll_bar_adjust_click_height_step;

		ipc_ptr->send_message("menu_plus", "get toggles", response);
	});

	ipc.map_function("set toggle", [ipc_ptr, settings_ptr](const string message_body)
	{
		const string toggle_name = message_body.substr(0, message_body.size() - 1);
		const string toggle_value = message_body.substr(message_body.size() - 1, message_body.size());

		if (toggle_name == "toggleLaunchOnStartup")
			settings_ptr->launch_on_startup = toggle_value;

		else if (toggle_name == "togglePowerSavingMode")
			settings_ptr->power_saving_mode = toggle_value;

		else if (toggle_name == "toggleCheckForUpdates")
			settings_ptr->check_for_updates = toggle_value;

		else if (toggle_name == "toggleTouchControl")
			settings_ptr->touch_control = toggle_value;

		else if (toggle_name == "toggleTableMode")
			settings_ptr->table_mode = toggle_value;

		else if (toggle_name == "toggleAutoDetectInteractionPlane")
			settings_ptr->auto_detect_interaction_plane = toggle_value;

		else if (toggle_name == "scrollBarAdjustClickHeightStep")
			settings_ptr->scroll_bar_adjust_click_height_step = toggle_value;

		ofstream settings_ofs(settings_file_path, ios::binary);
		settings_ofs.write((char*)settings_ptr, sizeof(*settings_ptr));

		ipc_ptr->send_message("menu_plus", "set toggle", "");
		ipc_ptr->send_message("track_plus", "load settings", "");
	});

	ipc.map_function("exit", [](const string message_body)
	{
		block_guardian = true;
		ipc.send_message("everyone", "exit", "");
		ipc.clear();
		exit(0);
	});

	thread guardian_thread(guardian_thread_function);
	thread ipc_thread(ipc_thread_function);

	while (true)
	{
		if (settings.launch_on_startup == "1" && !file_exists(get_startup_folder_path() + slash + "Touch+ Software.lnk"))
			create_shortcut(executable_path + slash + "daemon_plus.exe",
							get_startup_folder_path() + slash + "Touch+ Software.lnk",
							executable_path);

		else if (settings.launch_on_startup != "1" && file_exists(get_startup_folder_path() + slash + "Touch+ Software.lnk"))
			delete_file(get_startup_folder_path() + slash + "Touch+ Software.lnk");

		Sleep(20);
	}

	return 0;
}