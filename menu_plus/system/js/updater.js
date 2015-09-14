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

var Updater = function(ipcIn, s3In, menuIn)
{
	const self = this;

	self.ipc = ipcIn;
	self.s3 = s3In;
	self.menu = menuIn;

	setInterval(self.CheckForUpdate, 3600000);
};

Updater.prototype.Enabled = false;
Updater.prototype.NotificationHead = "";
Updater.prototype.NotificationBody = "";
Updater.prototype.ipc = null;
Updater.prototype.s3 = null;
Updater.prototype.menu = null;
Updater.prototype.checkingForUpdate = false;
Updater.prototype.manualInvoke = false;
Updater.prototype.patching = false;

Updater.prototype.CheckForUpdate = function(manual)
{
	const self = this;

	if (self.patching)
		return;

	if (manual == true)
	{
		self.manualInvoke = true;
		self.NotificationHead = "Checking for updates";
		self.NotificationBody = "Please wait";
		var notification = new Notification(self.NotificationHead,
			{ body: self.NotificationBody, icon: "file://" + process.cwd() + "system/images/ractiv.png" });
	}

	if (!self.Enabled && !manual)
		return;

	if (self.checkingForUpdate)
		return;

	self.checkingForUpdate = true;
	// console.log("checking for updates");

	self.s3.GetKeys(function(keys)
	{
		var found = false;
		for (var i in keys)
		{
			var key = keys[i];
			if (key.Key == "software_update/version.txt")
			{
				found = true;

				self.s3.ReadTextKey(key.Key, function(targetVersion)
				{
					var currentVersion = ReadTextFile(ExecutablePath + "/version.nrocinunerrad")[0];
					if (targetVersion != currentVersion)
					{
						if (self.manualInvoke)
							self.patch(targetVersion);
						else
						{
							self.NotificationHead = "A new version of Touch+ software is available";
							self.NotificationBody = "Click here to update";
							var notification = new Notification(self.NotificationHead,
								{ body: self.NotificationBody, icon: "file://" + process.cwd() + "system/images/ractiv.png" });

							notification.onclick = function()
							{
								self.patch(targetVersion);
							};
						}
					}
					else if (self.manualInvoke)
					{
						self.NotificationHead = "Touch+ software is up to date";
						self.NotificationBody = "Current vesion: " + currentVersion;
						var notification = new Notification(self.NotificationHead,
							{ body: self.NotificationBody, icon: "file://" + process.cwd() + "system/images/ractiv.png" });

						self.manualInvoke = false;
					}
					self.checkingForUpdate = false;
				},
				function()
				{
					if (self.manualInvoke)
					{
						self.NotificationHead = "Update failed";
						self.NotificationBody = "please check your internet connection";
						var notification = new Notification(self.NotificationHead,
							{ body: self.NotificationBody, icon: "file://" + process.cwd() + "system/images/ractiv.png" });
					}

					self.checkingForUpdate = false;
					// console.log("update failed");
				});

				break;
			}
		}

		if (!found)
			self.checkingForUpdate = false;
	},
	function()
	{
		if (self.manualInvoke)
		{
			self.NotificationHead = "Update failed";
			self.NotificationBody = "please check your internet connection";
			var notification = new Notification(self.NotificationHead,
				{ body: self.NotificationBody, icon: "file://" + process.cwd() + "system/images/ractiv.png" });
		}

		self.checkingForUpdate = false;
		// console.log("update failed");
	});
};

Updater.prototype.patch = function(targetVersion)
{
	const self = this;

	if (self.patching)
		return;

	self.patching = true;

	self.NotificationHead = "Downloading update";
	self.NotificationBody = "Please wait";
	var notification = new Notification(self.NotificationHead,
		{ body: self.NotificationBody, icon: "file://" + process.cwd() + "system/images/ractiv.png" });

	self.s3.DownloadKey("software_update/patch.zip", ExecutablePath, function(path)
	{
		self.NotificationHead = "Installing update";
		self.NotificationBody = "Please wait";
		var notification = new Notification(self.NotificationHead,
			{ body: self.NotificationBody, icon: "file://" + process.cwd() + "system/images/ractiv.png" });

		BlockExit = true;
		self.ipc.SendMessage("daemon_plus", "exit", "");

		setTimeout(function()
		{
			var AdmZip = require(process.cwd() + "/modules/adm-zip");
			new AdmZip(path).extractAllTo(ExecutablePath, true);
			DeleteFile(path);

			self.NotificationHead = "Update finished";
			self.NotificationBody = "Current version: " + targetVersion;
			var notification = new Notification(self.NotificationHead,
				{ body: self.NotificationBody, icon: "file://" + process.cwd() + "system/images/ractiv.png" });

			BlockExit = false;
			
			self.patching = false;
			self.manualInvoke = false;

			// console.log("update finished");

		}, 1000);
	},
	function()
	{
		if (self.manualInvoke == true)
		{
			self.NotificationHead = "Update failed";
			self.NotificationBody = "please check your internet connection";
			var notification = new Notification(self.NotificationHead,
				{ body: self.NotificationBody, icon: "file://" + process.cwd() + "system/images/ractiv.png" });

			self.manualInvoke = false;
		}

		self.patching = false;
		// console.log("update failed");
	},
	function(loaded, total)
	{
		menu.removeAt(1);
		menu.insert(new require("nw.gui").MenuItem({ type: "normal", label: loaded + " of " + total + " bytes downloaded" }), 1);
	});
}