#include <iostream>
#include "console_log.h"

using namespace std;

IPC* console_log_ipc = NULL;

void console_log(std::string str, bool do_log)
{
	if (console_log_ipc != NULL)
		console_log_ipc->send_message("menu_plus", "console log", str + "~@#~", do_log);

	cout << str << endl;
}

void console_log_inline(std::string str)
{
	if (console_log_ipc != NULL)
		console_log_ipc->send_message("menu_plus", "console log", str);

	cout << str;
}

void console_log_endline()
{
	if (console_log_ipc != NULL)
		console_log_ipc->send_message("menu_plus", "console log", "~@#~");

	cout << endl;
}