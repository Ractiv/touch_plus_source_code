#pragma once

#include <SFML/Network.hpp>

using namespace std;

class UDP
{
public:
	const sf::IpAddress address = "127.0.0.1";

	int port;

	sf::UdpSocket socket;

	void set_port(const int port_in);
	void send_message(const string message);
};