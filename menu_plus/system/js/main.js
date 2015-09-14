$(document).ready(function()
{
    $(window).resize(function()
    {
        window.resizeTo(640, 675);
        window.focus();
    });
});

//----------------------------------------------------------------------------------------------------

var gui;
var gui_ready = false;

var status = "hello world";
var loading_percent = 100;
var downloading_percent = 100;

function set_gui_ready()
{
	gui = document.getElementById("gui");
}

function get_var(var_name)
{
	return var_name;
}

swfobject.embedSWF("main.swf", "gui", "640", "675", "9.0.0");

//----------------------------------------------------------------------------------------------------

var ipc = new IPC("menu_plus");
var nw = require("nw.gui");
var tray = new nw.Tray({ icon: "./system/images/ractiv_tray.png" });
var menu = new nw.Menu();
var win = nw.Window.get();
var winShow = false;
var winShowOld = false;
var s3 = new S3();
var updater = new Updater(ipc, s3, menu);
  
// win.hide();

//----------------------------------------------------------------------------------------------------

function terminate()
{
	ipc.SendMessage("daemon_plus", "exit", "");

	setTimeout(function()
	{
		ipc.SendMessage("everyone", "exit", "");
		
	}, 1000);
}

function ShowNotification(notificationHead, notificationBody)
{
	var notification = new Notification(notificationHead,
		{ body: notificationBody, icon: "file://" + process.cwd() + "system/images/ractiv.png" });
}

ipc.MapFunction("//evaluate javascript", function(messageBody)
{
	var path = messageBody.replace("//", "");
	var str = ReadTextFileIntoString(path);
	eval(str);
});

//----------------------------------------------------------------------------------------------------

menu.append(new nw.MenuItem({ type: "normal", label: "Show control panel", click: function()
{
	win.show();
	winShow = true;
}}));

menu.append(new nw.MenuItem({ type: "normal", label: "Update software", click: function()
{
	updater.CheckForUpdate(true);
}}));

menu.append(new nw.MenuItem({ type: "normal", label: "Exit", click: function()
{
	terminate();
}}));

tray.menu = menu;

// win.on("close", function()
// {
// 	win.hide();
// 	winShow = false;
// });

win.on("minimize", function()
{
	win.hide();
	winShow = false;
});

tray.on("click", function()
{
	win.show();
	winShow = true;
});

//----------------------------------------------------------------------------------------------------

ipc.MapFunction("menu_plus_ready", function(messageBody)
{
	ipc.SendMessage("track_plus", "menu_plus_ready", "");
});

ipc.MapFunction("show window", function(messageBody)
{
	win.show();
	winShow = true;
});

ipc.MapFunction("hide window", function(messageBody)
{
	win.hide();
	winShow = false;
});

ipc.MapFunction("show notification", function(messageBody)
{
	var str_vec = messageBody.split(":");
	var notificationHead = str_vec[0];
	var notificationBody = str_vec[1];
	var notification = new Notification(notificationHead,
		{ body: notificationBody, icon: "file://" + process.cwd() + "system/images/ractiv.png" });
});

ipc.MapFunction("exit", function(messageBody)
{
	if (BlockExit)
		return;

	ipc.Clear();
	DeleteFolder(nw.App.dataPath);
	process.exit(0);
});

ipc.MapFunction("download failed", function(messageBody)
{
	alert("Download failed, cannot connect to server");
	terminate();
});

ipc.MapFunction("download", function(messageBody)
{
	var urlPath = messageBody.split("`");
	var url = urlPath[0];
	var path = urlPath[1];

	var http = require("http");
	var fs = require("fs");

	var file = fs.createWriteStream(path);
	var request = http.get(url, function(response)
	{
		response.pipe(file);
		file.on("finish", function()
		{
			file.close();
			ipc.SendMessage("track_plus", "download", "true");
		});

	}).on("error", function(err)
	{
		alert("Download failed, cannot connect to server");
		ipc.SendMessage("track_plus", "download", "false");
	});
});

//----------------------------------------------------------------------------------------------------

setInterval(ipc_loop, 100);
function ipc_loop()
{
	ipc.Update();
}