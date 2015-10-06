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

#include "ipc.h"
// #include "console_log.h"

IPC::IPC(string self_name_in)
{
	self_name = self_name_in;
	update();
}

void IPC::update()
{
	static bool updated = true;

	if (!updated)
		return;

	updated = false;

	static unordered_map<string, bool> file_name_processed_map;

	vector<string> file_name_vec = list_files_in_directory(ipc_path);
	for (string file_name_current : file_name_vec)
	{
		string file_name_lock = "";
		if (file_name_current.size() >= 4)
			file_name_lock = file_name_current.substr(0, 4);

		if (file_name_lock == "lock")
		{
			updated = true;
			return;
		}
	}

	for (string file_name_current : file_name_vec)
	{
		string file_name_everyone = "";
		if (file_name_current.size() >= 8)
			file_name_everyone = file_name_current.substr(0, 8);

		if (file_name_current.size() > self_name.size() || file_name_everyone == "everyone")
		{
			if (file_name_processed_map[file_name_current] == true)
					continue;
				else
					file_name_processed_map[file_name_current] = true;

			string file_name = "";
			string file_name_id_str = "";
			if (file_name_everyone != "everyone")
			{
				file_name = file_name_current.substr(0, self_name.size());
				file_name_id_str = file_name_current.substr(self_name.size(), file_name_current.size());
			}

			if (file_name == self_name || file_name_everyone == "everyone")
			{
				Sleep(20);

				vector<string> lines = read_text_file(ipc_path + slash + file_name_current);
				// delete_file(ipc_path + slash + file_name_current);
				vector<string> message_vec = split_string(lines[0], "!");
				string message_head = message_vec[0];
				string message_body = message_vec[1];

				// console_log("message_received " + message_head + " " + message_body + " " + file_name_current, false);

				if (response_map.count(message_head) == 0)
				{
					if (command_map.count(message_head))
						command_map[message_head](message_body);
				}
				else
				{
					function<void (string)> func = response_map[message_head];
					response_map.erase(message_head);
					func(message_body);
				}
			}
		}
	}
		
	updated = true;
}

void IPC::clear()
{
	vector<string> file_name_vec = list_files_in_directory(ipc_path);
	for (string file_name_current : file_name_vec)
	{
		string file_name_everyone = "";
		if (file_name_current.size() >= 8)
			file_name_everyone = file_name_current.substr(0, 8);

		if (file_name_current.size() > self_name.size() || file_name_everyone == "everyone")
		{
			string file_name = "";
			string file_name_id_str = "";
			if (file_name_everyone != "everyone")
			{
				file_name = file_name_current.substr(0, self_name.size());
				file_name_id_str = file_name_current.substr(self_name.size(), file_name_current.size());
			}
			else
				continue;

			if (file_name == self_name || file_name_everyone == "everyone")
				delete_file(ipc_path + slash + file_name_current);
		}
	}
}

void IPC::send_message(string recipient, string message_head, string message_body, bool do_log)
{
	static int lock_file_count = 0;
	string lock_file_name = "lock_" + self_name + to_string(lock_file_count);
	write_string_to_file(ipc_path + slash + lock_file_name, "");
	++lock_file_count;

	vector<string> file_name_vec = list_files_in_directory(ipc_path);

	bool found = true;
	int file_count = 0;

	while (found)
	{
		found = false;
		for (string file_name_current : file_name_vec)
			if (file_name_current == recipient + to_string(file_count))
			{
				found = true;
				++file_count;
				break;
			}
	}

	static int sent_count = 0;

	string path_new = ipc_path + slash + recipient + to_string(file_count);
	write_string_to_file(path_new, message_head + "!" + message_body);

	++sent_count;
	delete_file(ipc_path + slash + lock_file_name);
	
	// if (do_log)
		// console_log("message sent: " + recipient + " " + message_head + " " + message_body, false);
}

void IPC::get_response(string recipient, string message_head, string message_body,
					   function<void (string message_body)> callback)
{
	send_message(recipient, message_head, message_body);
	response_map[message_head] = callback;
}

void IPC::map_function(string message_head, function<void (string message_body)> callback)
{
	command_map[message_head] = callback;
}

void IPC::open_udp_channel(string recipient, int port_num)
{
	udp_map[recipient] = &(udp_pool[udp_pool_index]);
	++udp_pool_index;
	UDP* udp_ptr = udp_map[recipient];

	bool file_found = false;

	vector<string> file_name_vec = list_files_in_directory(ipc_path);
	for (string file_name_current : file_name_vec)
		if (file_name_current == "udp_port")
			file_found = true;

	if (!file_found)
	{
		if (port_num == -1)
		{
			int port_old = 0;
			int port_new = 0;

			int* port_new_ptr = &port_new;
			get_response(recipient, "open udp channel", "", [udp_ptr, port_new_ptr](string message_body)
			{
				int port = atoi(message_body.c_str());
				udp_ptr->set_port(port);
				*port_new_ptr = port;

				// console_log("udp port is " + to_string(port), false);
			});

			while (port_old == port_new)
			{
				Sleep(100);
				update();
			}
		}
		else
		{
			send_message(recipient, "open udp channel", to_string(port_num));
			int port = port_num;
			udp_ptr->set_port(port);

			// console_log("udp port is " + to_string(port), false);
		}
	}
	else
	{
		int port = atoi(read_text_file(ipc_path + slash + "udp_port")[0].c_str());
		udp_ptr->set_port(port);

		// console_log("udp port is " + to_string(port), false);
	}
}

void IPC::send_udp_message(string recipient, string message)
{
	if (udp_map.count(recipient) > 0)
		udp_map[recipient]->send_message(message);
}

void IPC::run_js(vector<string> lines)
{
	string recipient = "menu_plus";
	string message_head = "//evaluate javascript";
	string message_body = "";
	for(string& line : lines)
		message_body += line + "\n";

	static int lock_file_count = 0;
	string lock_file_name = "lock_" + self_name + to_string(lock_file_count);
	write_string_to_file(ipc_path + slash + lock_file_name, "");
	++lock_file_count;

	vector<string> file_name_vec = list_files_in_directory(ipc_path);

	bool found = true;
	int file_count = 0;

	while (found)
	{
		found = false;
		for (string file_name_current : file_name_vec)
			if (file_name_current == recipient + to_string(file_count))
			{
				found = true;
				++file_count;
				break;
			}
	}

	static int sent_count = 0;

	string path_new = ipc_path + slash + recipient + to_string(file_count);

	message_body = "//" + path_new + "\n" + message_body;
	write_string_to_file(path_new, message_head + "!" + message_body);

	++sent_count;
	delete_file(ipc_path + slash + lock_file_name);
	
	// console_log("message sent: " + recipient + " " + message_head + " " + message_body, false);
}