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

var http = require("http");
var fs = require("fs");
var path = require("path");
var net = require("net");
var nw = require("nw.gui");

var portRange = 45032;
var server = null;

//----------------------------------------------------------------------------------------------------
//do some cleanup before exit

process.on("exit", function()
{
	server.close();
});

//----------------------------------------------------------------------------------------------------

function getPort(cb)
{
	var port = portRange;
	portRange -= 1;

	server = net.createServer();
	server.listen(port, function(err)
	{
		server.once("close", function()
		{
			cb(port);
		});
		server.close();
	});

	server.on("error", function(err)
	{
		getPort(cb);
	});
}

getPort(function(port)
{
	http.createServer(function (request, response)
	{
		console.log("request starting...");

		var filePath = "." + request.url;
		if (filePath == "./")
		    filePath = "./index.html";

		var extname = path.extname(filePath);
		var contentType = "text/html";
		switch (extname)
		{
		    case ".js":
		        contentType = "text/javascript";
		        break;
		    case ".css":
		        contentType = "text/css";
		        break;
		    case ".json":
		        contentType = "application/json";
		        break;
		    case ".png":
		        contentType = "image/png";
		        break;      
		    case ".jpg":
		        contentType = "image/jpg";
		        break;
		    case ".wav":
		        contentType = "audio/wav";
		        break;
		}

		fs.readFile(filePath, function(error, content)
		{
		    if (error)
		    {
		        if(error.code == "ENOENT")
		        {
		            fs.readFile("./404.html", function(error, content)
		            {
		                response.writeHead(200, { "Content-Type": contentType });
		                response.end(content, "utf-8");
		            });
		        }
		        else
		        {
		            response.writeHead(500);
		            response.end("an error has occurred: " + error.code + " ..\n");
		            response.end(); 
		        }
		    }
		    else {
		        response.writeHead(200, { "Content-Type": contentType });
		        response.end(content, "utf-8");
		    }
		});

	}).listen(port);

	var url = "http://127.0.0.1:" + port;
	console.log("Server running at " + url);

	var iframe = document.createElement("iframe");
	iframe.src = url + "/system/main.html";
	iframe.width = "100%";
	iframe.height = "100%";
	iframe.frameBorder = "0";
	document.body.appendChild(iframe);
});