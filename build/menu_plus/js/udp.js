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

var UDP = function() { };

UDP.prototype.callbackSet = false;
UDP.prototype.udpCallback = null;

UDP.prototype.Assign = function()
{
	var self = this;

	var dgram = require("dgram");
	var server = dgram.createSocket("udp4");

	server.on("message", function (message, remote)
	{
		self.dataReceived(message);
	});

	var port = 3333;
	server.bind(port, "127.0.0.1");
	return port;
};

UDP.prototype.dataReceived = function(message)
{
	var self = this;

	if (self.callbackSet)
		self.udpCallback(message);
}

UDP.prototype.SetUDPCallback = function(udpCallbackIn)
{
	var self = this;

	self.udpCallback = udpCallbackIn;
	self.callbackSet = true;
}