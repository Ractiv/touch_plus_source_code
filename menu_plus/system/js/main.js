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
	gui_ready = true;
}

function get_var(var_name)
{
	return var_name;
}

swfobject.embedSWF("main.swf", "gui", "640", "675", "9.0.0");

//----------------------------------------------------------------------------------------------------

const ipc = new IPC("menu_plus");
const nw = require("nw.gui");
const tray = new nw.Tray({ icon: "./system/images/ractiv_tray.png" });
const menu = new nw.Menu();
const win = nw.Window.get();
const winShow = false;
const winShowOld = false;
const s3 = new S3();
const updater = new Updater(ipc, s3, menu);
  
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
	const notification = new Notification(notificationHead,
		{ body: notificationBody, icon: "file://" + process.cwd() + "system/images/ractiv.png" });
}

ipc.MapFunction("//evaluate javascript", function(messageBody)
{
	const path = messageBody.replace("//", "");
	const str = ReadTextFileIntoString(path);
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
	const str_vec = messageBody.split(":");
	const notificationHead = str_vec[0];
	const notificationBody = str_vec[1];
	const notification = new Notification(notificationHead,
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
	const urlPath = messageBody.split("`");
	const url = urlPath[0];
	const path = urlPath[1];

	const http = require("http");
	const fs = require("fs");

	const file = fs.createWriteStream(path);
	const request = http.get(url, function(response)
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

//----------------------------------------------------------------------------------------------------

function toggle_clicked(toggle_name, toggle_state)
{
	console.log(toggle_name + " " + toggle_state);
}

function call_as(cb)
{
	const interval = setInterval(function()
	{
		if (gui_ready)
		{
			clearInterval(interval);
			cb();
		}
	}, 100);
}

function switch_toggle(toggle_name)
{
	call_as(function()
	{
		gui.switch_toggle(toggle_name);
	});
}

switch_toggle("launch_on_startup");