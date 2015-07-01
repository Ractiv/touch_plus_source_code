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

IPC::IPC(const string self_name_in)
{
	self_name = self_name_in;
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
		if (file_name_current.size() > self_name.size())
		{
			if (file_name_processed_map[file_name_current] == true)
					continue;
				else
					file_name_processed_map[file_name_current] = true;

			const string file_name = file_name_current.substr(0, self_name.size());
			const string file_name_id_str = file_name_current.substr(self_name.size(), file_name_current.size());

			if (file_name == self_name)
			{
				Sleep(10);

				vector<string> lines = read_text_file(ipc_path + "\\" + file_name_current);
				// delete_file(ipc_path + "\\" + file_name_current);
				vector<string> message_vec = split_string(lines[0], "!");
				const string message_head = message_vec[0];
				const string message_body = message_vec[1];

				COUT << "message_received " << message_head << " " << message_body << " " << file_name_current << endl;

				if (response_map.count(message_head) == 0)
				{
					if (command_map.count(message_head))
						command_map[message_head](message_body);
				}
				else
				{
					function<void (const string)> func = response_map[message_head];
					response_map.erase(message_head);
					func(message_body);
				}
			}
		}
		
	updated = true;
}

void IPC::send_message(const string recipient, const string message_head, const string message_body)
{
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

	const string path_old = ipc_path + "\\s" + self_name + to_string(sent_count);
	const string path_new = ipc_path + "\\" + recipient + to_string(file_count);

	write_string_to_file(path_old, message_head + "!" + message_body);
	rename_file(path_old, path_new);

	++sent_count;

	COUT << "message sent: " << recipient << " " << message_head << " " << message_body << endl;
}

void IPC::get_response(const string recipient, const string message_head, const string message_body,
					   function<void (const string message_body)> callback)
{
	send_message(recipient, message_head, message_body);
	response_map[message_head] = callback;
}

void IPC::map_function(const string message_head, function<void (const string message_body)> callback)
{
	command_map[message_head] = callback;
}

void IPC::open_udp_channel(const string recipient)
{
	if (udp_map.count(recipient) == 0)
	{
		udp_map[recipient] = &(udp_pool[udp_pool_index]);
		++udp_pool_index;
	}

	int port_old = 0;
	int port_new = 0;
	int* port_new_ptr = &port_new;

	UDP* udp_ptr = udp_map[recipient];
	get_response(recipient, "open udp channel", "", [udp_ptr, port_new_ptr](const string message_body)
	{
		const int port = atoi(message_body.c_str());
		udp_ptr->set_port(port);

		COUT << "udp port is " << port << endl;
		*port_new_ptr = port;
	});

	while (port_old == port_new)
	{
		Sleep(20);
		update();
	}
}

void IPC::send_udp_message(const string recipient, const string message)
{
	if (udp_map.count(recipient) > 0)
		udp_map[recipient]->send_message(message);
}