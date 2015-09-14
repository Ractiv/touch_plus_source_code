const http = require("http");
const fs = require("fs");
const path = require("path");
const net = require("net");
const nw = require("nw.gui");

var portRange = 45032;

function getPort(cb)
{
	const port = portRange;
	portRange += 1;

	const server = net.createServer();
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

		const filePath = "." + request.url;
		if (filePath == "./")
		    filePath = "./index.html";

		const extname = path.extname(filePath);
		const contentType = "text/html";
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

	const url = "http://127.0.0.1:" + port;
	console.log("Server running at " + url);

	const iframe = document.createElement("iframe");
	iframe.src = url + "/system/main.html";
	iframe.width = "100%";
	iframe.height = "100%";
	iframe.frameBorder = "0";
	document.body.appendChild(iframe);
});