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