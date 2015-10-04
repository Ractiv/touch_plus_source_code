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

#include <iostream>
#include <functional>
#include <unordered_map>
#include "globals.h"
#include "filesystem.h"
#include "string_functions.h"
#include "udp.h"

using namespace std;

class IPC
{
public:
	string self_name;

	unordered_map<string, function<void (string)>> response_map;

	unordered_map<string, function<void (string)>> command_map;

	unordered_map<string, UDP*> udp_map;

	UDP udp_pool[1000];

	int udp_pool_index = 0;

	IPC(string self_name_in);
	void update();
	void clear();
	void send_message(string recipient, string message_head, string message_body, bool do_log = true);
	void get_response(string recipient, string message_head, string message_body, function<void (string message_body)> callback);
	void map_function(string message_head, function<void (string message_body)> callback);
	void open_udp_channel(string recipient, int port_num = -1);
	void send_udp_message(string recipient, string message);
	void run_js(vector<string> lines);
};