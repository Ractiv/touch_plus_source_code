#include "udp.h"

void UDP::set_port(const int port_in)
{
	port = port_in;
}

void UDP::send_message(const string message)
{
	socket.send(message.c_str(), message.size() + 1, address, port);
}