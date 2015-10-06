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

/*$(document).ready(function()
{
    $(window).resize(function()
    {
        window.resizeTo(640, 675);
        window.focus();
    });
});*/

//----------------------------------------------------------------------------------------------------

var gui;
var gui_ready = false;

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

function show_page(index)
{
	call_as(function()
	{
		gui.show_page(index);
	});
}

//----------------------------------------------------------------------------------------------------

function win_show()
{
	win.show();
}

function win_hide()
{
	win.hide();
}

//----------------------------------------------------------------------------------------------------

var ipc = new IPC("menu_plus");
var nw = require("nw.gui");
var tray = new nw.Tray({ icon: "./system/images/ractiv_tray.png" });
var menu = new nw.Menu();
var win = nw.Window.get();
var s3 = new S3();
var updater = new Updater(ipc, s3, menu);
  
// win_hide();

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

ipc.MapFunction("//evaluate javascript", function(messageBody)
{
	var path = messageBody.replace("//", "");
	var str = ReadTextFileIntoString(path);
	eval(str);
});

//----------------------------------------------------------------------------------------------------

menu.append(new nw.MenuItem({ type: "normal", label: "Show control panel", click: function()
{
	win_show();
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

win.on("close", function()
{
	win_hide();
});

win.on("minimize", function()
{
	win_hide();
});

tray.on("click", function()
{
	win_show();
});

//----------------------------------------------------------------------------------------------------

ipc.MapFunction("menu_plus_ready", function(messageBody)
{
	ipc.SendMessage("track_plus", "menu_plus_ready", "");
});

ipc.MapFunction("show window", function(messageBody)
{
	win_show();
});

ipc.MapFunction("hide window", function(messageBody)
{
	win_hide();
});

ipc.MapFunction("show notification", function(messageBody)
{
	var str_vec = messageBody.split(":");
	show_notification(str_vec[0], str_vec[1]);
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
	// alert("Download failed, cannot connect to server");
	// terminate();
});

var downloaded_count = 0;
var downloaded_count_total = 3;

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
		var len = parseInt(response.headers["content-length"], 10);
        var cur = 0;
        response.on("data", function(chunk)
        {
            cur += chunk.length;
            var percent = (downloaded_count * 100.0 / downloaded_count_total) + (100.0 * cur / len / downloaded_count_total);

            call_as(function()
			{
				gui.set_downloading_progress(percent);
			});
        });

		response.pipe(file);
		file.on("finish", function()
		{
			++downloaded_count;
			file.close();
			ipc.SendMessage("track_plus", "download", "true");
		});

	}).on("error", function(err)
	{
		// alert("Download failed, cannot connect to server");
		ipc.SendMessage("track_plus", "download", "false");
	});
});

ipc.MapFunction("reset progress", function(messageBody)
{
	call_as(function()
	{
		gui.reset_progress();
	});
});

ipc.MapFunction("set downloading complete", function(messageBody)
{
	call_as(function()
	{
		gui.set_downloading_progress(100);
	});
});

ipc.MapFunction("set loading progress", function(messageBody)
{
	call_as(function()
	{
		gui.set_loading_progress(parseInt(messageBody));
	});
});

ipc.MapFunction("set status", function(messageBody)
{
	call_as(function()
	{
		gui.set_status(messageBody);
	});

	if (messageBody.substring(0, 5) == "error")
	{
		call_as(function()
		{
			console.log("hahaha here");
			gui.error_screen_on();
		});
	}
	else
	{
		call_as(function()
		{
			gui.error_screen_off();
		});
	}
});

ipc.MapFunction("show settings page", function(messageBody)
{
	show_page(0);
});

ipc.MapFunction("show download page", function(messageBody)
{
	show_page(4);
    ipc.SendMessage("track_plus", "show download page", "");
});

ipc.MapFunction("show debug page", function(messageBody)
{
	show_page(4);
});

ipc.MapFunction("console log", function(messageBody)
{
	call_as(function()
	{
		gui.console_log(messageBody);
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

function set_scroll_bar(scroll_bar_name, level)
{
	call_as(function()
	{
		gui.set_scroll_bar(scroll_bar_name, level);
	});
}

function scroll_bar_moved(scroll_bar_name, level)
{
	ipc.SendMessage("daemon_plus", "set toggle", scroll_bar_name + level.toString());
}

//----------------------------------------------------------------------------------------------------

function toggle_imshow()
{
	ipc.SendMessage("track_plus", "toggle imshow", "");
}

//----------------------------------------------------------------------------------------------------

ipc.GetResponse("daemon_plus", "get toggles", "", function(messageBody)
{
	var launch_on_startup = messageBody.substring(0, 1) == "1";
	var power_saving_mode = messageBody.substring(1, 2) == "1";
	var check_for_updates = messageBody.substring(2, 3) == "1";
	var touch_control = messageBody.substring(3, 4) == "1";
	var click_height = parseInt(messageBody.substring(6, 7));

	switch_toggle("launch_on_startup", launch_on_startup);
	switch_toggle("power_saving_mode", power_saving_mode);
	switch_toggle("check_for_updates", check_for_updates);
	switch_toggle("touch_control", touch_control);

	set_scroll_bar("click_height", click_height);
});

//----------------------------------------------------------------------------------------------------

function send_feedback(email, message)
{
	s3.GetKeys(function(keys)
	{
		var count = 0;

		for (var keyIndex in keys)
		{
			var key = keys[keyIndex];
			var keyPath = key.Key.split("/");

			if (keyPath[0] == "bug_report" && keyPath[1] == email && keyPath[2].substring(0, 1) == "m")
				++count;
		}

		s3.WriteTextKey("bug_report/" + email + "/message" + count + ".txt", message, function()
		{
			call_as(function()
			{
				gui.message_sent();
			});
		});
	},
	function()
	{
		call_as(function()
		{
			gui.message_failed();
		});
	});
}