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

	unordered_map<string, function<void (const string)>> response_map;

	unordered_map<string, function<void (const string)>> command_map;

	unordered_map<string, UDP*> udp_map;

	UDP udp_pool[1000];

	int udp_pool_index = 0;

	IPC(const string self_name_in);
	void update();
	void send_message(const string recipient, const string message_head, const string message_body);
	void get_response(const string recipient, const string message_head, const string message_body, 
					  function<void (const string message_body)> callback);
	void map_function(const string message_head, function<void (const string message_body)> callback);
	void open_udp_channel(const string recipient);
	void send_udp_message(const string recipient, const string message);
};