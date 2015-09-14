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

function call_as(cb)
{
	var interval = setInterval(function()
	{
		if (gui_ready)
		{
			clearInterval(interval);
			cb();
		}
	}, 100);
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
	console.log("terminate");
	ipc.SendMessage("daemon_plus", "exit", "");

	setTimeout(function()
	{
		console.log("force terminate");
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

//----------------------------------------------------------------------------------------------------

function switch_toggle(toggle_name, is_on)
{
	call_as(function()
	{
		gui.switch_toggle(toggle_name, is_on);
	});
}

function toggle_clicked(toggle_name, is_on)
{
	var state = is_on ? "0" : "1";
	ipc.GetResponse("daemon_plus", "set toggle", toggle_name + state, function(messageBody)
    {
     	switch_toggle(toggle_name, !is_on);
    });
}

//----------------------------------------------------------------------------------------------------

ipc.GetResponse("daemon_plus", "get toggles", "", function(messageBody)
{
	var launch_on_startup = messageBody.substring(0, 1) == "1";
	var power_saving_mode = messageBody.substring(1, 2) == "1";
	var check_for_updates = messageBody.substring(2, 3) == "1";
	var touch_control = messageBody.substring(3, 4) == "1";
	var scroll_bar_adjust_click_height_step = parseInt(messageBody.substring(6, 7));

	switch_toggle("launch_on_startup", launch_on_startup);
	switch_toggle("power_saving_mode", power_saving_mode);
	switch_toggle("check_for_updates", check_for_updates);
	switch_toggle("touch_control", touch_control);

	// setScrollBar($scrollBar, 195, 431, 0, 9, scrollBarAdjustClickHeightStep, 6, "scrollBarAdjustClickHeightStep");
});