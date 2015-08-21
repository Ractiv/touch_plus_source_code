var ipc = new IPC("menu_plus");
var gui = require("nw.gui");
var tray = new gui.Tray({ icon: "./ractiv_tray.png" });
var menu = new gui.Menu();
var win = gui.Window.get();
var winShow = false;
var winShowOld = false;
  
win.hide();

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
	var notification = new Notification(notificationHead, { body: notificationBody, icon: "file://" + process.cwd() + "/ractiv.png" });
}