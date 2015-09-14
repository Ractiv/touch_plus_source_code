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

UDP.prototype.port = 3333;
UDP.prototype.address = "127.0.0.1";
UDP.prototype.socket = null;
UDP.prototype.callbackSet = false;
UDP.prototype.udpCallback = null;

UDP.prototype.Assign = function(portIn)
{
	var self = this;

	portIn = typeof portIn !== "undefined" ? portIn : -1;

	var dgram = require("dgram");
	self.socket = dgram.createSocket("udp4");

	var fileFound = false;
	var fileName = "";

	var fileNameVec = ListFilesInDirectory(IpcPath);
	for (var fileNameCurrentIndex in fileNameVec)
    {
    	var fileNameCurrent = fileNameVec[fileNameCurrentIndex];
    	if (fileNameCurrent == "udp_port")
    	{
    		fileFound = true;
    		fileName = fileNameCurrent;
    		break;
    	}
    }

    if (fileFound)
    {
    	var udpPortStr = ReadTextFile(IpcPath + "\\udp_port")[0];
    	self.port = parseInt(udpPortStr);
    }
    else
    {
    	if (portIn == -1)
			self.port = 3333;
		else
			self.port = portIn;
    }

	self.socket.bind(self.port, self.address);

	self.socket.on("error", function(err)
	{
	  	console.log("Socket error: " + err.message);

	}).on("listening", function()
	{
	  	console.log("Successfully bound to socket!");
	});

	self.socket.on("message", function (message, remote)
	{
		self.dataReceived(message);
	});

	return self.port;
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

UDP.prototype.SendMessage = function(message)
{
	var self = this;

	var buffer = new Buffer(message);
	self.socket.send(buffer, 0, buffer.length, self.port, self.address);
}