#pragma once

#include <SFML/Network.hpp>

using namespace std;
using namespace sf;

class UDP
{
public:
	const IpAddress address = "127.0.0.1";

	int port;

	UdpSocket socket;

	void set_port(const int port_in);
	void send_message(const string message);
};