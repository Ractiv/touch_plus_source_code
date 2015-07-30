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

var Updater = function(ipcIn, s3In)
{
	var self = this;

	self.ipc = ipcIn;
	self.s3 = s3In;
	setInterval(self.Run, 3600000);
};

Updater.prototype.Running = false;
Updater.prototype.NotificationHead = "";
Updater.prototype.NotificationBody = "";
Updater.prototype.ipc = null;
Updater.prototype.s3 = null;
Updater.prototype.waitingForAction = false;
Updater.prototype.Enabled = true;

Updater.prototype.Run = function(manual)
{
	var self = this;

	if (!self.Enabled)
		return;

	if (manual == true && !self.waitingForAction)
	{
		self.NotificationHead = "Checking for updates";
		self.NotificationBody = "Please wait";
		var notification = new Notification(self.NotificationHead, { body: self.NotificationBody });
	}

	if (self.Running)
		return;

	self.Running = true;
	console.log("checking for updates");

	self.s3.GetKeys(function(keys)
	{
		for (var i in keys)
		{
			var key = keys[i];

			if (key.Key == "software_update/version.txt")
			{
				self.s3.ReadTextKey(key.Key, function(targetVersion)
				{
					var currentVersion = ReadTextFile(ExecutablePath + "/version.nrocinunerrad")[0];
					if (targetVersion != currentVersion)
					{
						if (manual == true)
							patch();
						else
						{
							self.waitingForAction = true;

							self.NotificationHead = "A new version of Touch+ software is available";
							self.NotificationBody = "Click here to update";
							var notification = new Notification(self.NotificationHead, { body: self.NotificationBody });

							notification.onclick = function()
							{
								self.waitingForAction = false;
								patch();
							};

							notification.onclose = function()
							{
								self.waitingForAction = false;
								self.Running = false;
							};
						}

						function patch()
						{
							self.NotificationHead = "Downloading update";
							self.NotificationBody = "Please wait";
							var notification = new Notification(self.NotificationHead, { body: self.NotificationBody });

							self.s3.DownloadKey("software_update/patch.zip", ExecutablePath, function(path)
							{
								self.NotificationHead = "Installing update";
								self.NotificationBody = "Please wait";
								var notification = new Notification(self.NotificationHead, { body: self.NotificationBody });

								BlockExit = true;
								self.ipc.SendMessage("daemon_plus", "exit", "");

								setTimeout(function()
								{
									var AdmZip = require("./modules/adm-zip/adm-zip");
									new AdmZip(path).extractAllTo(ExecutablePath, true);
									DeleteFile(path);

									// self.ipc.SendMessage("daemon_plus", "start", "");

									self.NotificationHead = "Update finished";
									self.NotificationBody = "Current version: " + targetVersion;
									var notification = new Notification(self.NotificationHead, { body: self.NotificationBody });

									self.Running = false;
									BlockExit = false;
									
									console.log("update finished");
								}, 1000);
							},
							function()
							{
								if (manual == true)
								{
									self.NotificationHead = "Update failed";
									self.NotificationBody = "please check your internet connection";
									var notification = new Notification(self.NotificationHead, { body: self.NotificationBody });
								}

								self.Running = false;
								console.log("update failed");
							});
						}
					}
					else
					{
						if (manual == true)
						{
							self.NotificationHead = "Touch+ software is already up to date";
							self.NotificationBody = "Current version: " + currentVersion;
							var notification = new Notification(self.NotificationHead, { body: self.NotificationBody });
						}

						self.Running = false;
						console.log("already up to date");
					}
				},
				function()
				{
					if (manual == true)
					{
						self.NotificationHead = "Update failed";
						self.NotificationBody = "please check your internet connection";
						var notification = new Notification(self.NotificationHead, { body: self.NotificationBody });
					}

					self.Running = false;
					console.log("update failed");
				});

				break;
			}
		}
	},
	function()
	{
		if (manual == true)
		{
			self.NotificationHead = "Update failed";
			self.NotificationBody = "please check your internet connection";
			var notification = new Notification(self.NotificationHead, { body: self.NotificationBody });
		}

		self.Running = false;
		console.log("update failed");
	});
};

Updater.prototype.runcmd = function(executable, args, callback)
{
    var spawn = require("child_process").spawn;
    console.log(executable, args);
    var child = spawn(executable, args);
    
    child.stdout.on("data", function(chunk)
    {
        if (typeof callback === "function")
            callback(chunk);
    });

    child.stderr.on("data", function (data)
    {
        console.log("stderr: " + data);
    });
};