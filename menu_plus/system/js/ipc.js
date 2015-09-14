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

const IPC = function(selfNameIn)
{
	const self = this;

	self.selfName = selfNameIn;

	self.MapFunction("open udp channel", function(messageBody)
	{
		if (messageBody == "")
		{
			const port = self.udp.Assign();
			self.SendMessage("track_plus", "open udp channel", port.toString());
		}
		else
		{
			const port = parseInt(messageBody);
			self.udp.Assign(port);
		}

	});
	
	self.Update();
};

IPC.Updated = 0;
IPC.FileNameProcessedMap = new Array();
IPC.SentCount = 0;
IPC.LockFileCount = 0;

IPC.prototype.selfName = "";
IPC.prototype.responseMap = new Array();
IPC.prototype.commandMap = new Array();
IPC.prototype.udp = new UDP();

IPC.prototype.Update = function()
{
	const self = this;

	if (IPC.Updated > 0)
		return;

	const fileNameVec = ListFilesInDirectory(IpcPath);
	for (var fileNameVecIndex in fileNameVec)
	{
		var fileNameCurrent = fileNameVec[fileNameVecIndex];
		var fileNameLock = "";
		if (fileNameCurrent.length >= 4)
			fileNameLock = fileNameCurrent.substring(0, 4);

		if (fileNameLock == "lock")
		{
			IPC.Updated = 0;
			return;
		}
	}

	for (var fileNameVecIndex in fileNameVec)
	{
		var fileNameCurrent = fileNameVec[fileNameVecIndex];

		var fileNameEveryone = "";
		if (fileNameCurrent.length >= 8)
			fileNameEveryone = fileNameCurrent.substring(0, 8);

		if (fileNameCurrent.length > self.selfName.length || fileNameEveryone == "everyone")
		{
			if (IPC.FileNameProcessedMap[fileNameCurrent] == true)
				continue;
			else
				IPC.FileNameProcessedMap[fileNameCurrent] = true;

			var fileName = "";
			var fileNameID = "";
			if (fileNameEveryone != "everyone")
			{
				fileName = fileNameCurrent.substring(0, self.selfName.length);
				fileNameID = fileNameCurrent.substring(self.selfName.length, fileNameCurrent.length);
			}
			
			if (fileName == self.selfName || fileNameEveryone == "everyone")
			{
				++IPC.Updated;

				const lines = ReadTextFile(IpcPath + "/" + fileNameCurrent);
				// DeleteFile(IpcPath + "/" + fileNameCurrent);

				const messageVec = lines[0].split("!");
				const messageHead = messageVec[0];
				const messageBody = messageVec[1];

				//console.log("message received " + messageHead + " " + messageBody + " " + fileNameCurrent + " " + fileName);

				if (self.responseMap[messageHead] == null)
				{
					if (self.commandMap[messageHead] != null)
						self.commandMap[messageHead](messageBody);
				}
				else
				{
					const func = self.responseMap[messageHead];
					self.responseMap[messageHead] = null;
					func(messageBody);
				}

				--IPC.Updated;
			}
		}
	}
};

IPC.prototype.Clear = function()
{
	const self = this;

	const fileNameVec = ListFilesInDirectory(IpcPath);
	for (var fileNameVecIndex in fileNameVec)
	{
		const fileNameCurrent = fileNameVec[fileNameVecIndex];

		var fileNameEveryone = "";
		if (fileNameCurrent.length >= 8)
			fileNameEveryone = fileNameCurrent.substring(0, 8);

		if (fileNameCurrent.length > self.selfName.length || fileNameEveryone == "everyone")
		{
			var fileName = "";
			var fileNameID = "";
			if (fileNameEveryone != "everyone")
			{
				fileName = fileNameCurrent.substring(0, self.selfName.length);
				fileNameID = fileNameCurrent.substring(self.selfName.length, fileNameCurrent.length);
			}
			else
				continue;
			
			if (fileName == self.selfName || fileNameEveryone == "everyone")
				DeleteFile(IpcPath + "/" + fileNameCurrent);
		}
	}
};

IPC.prototype.SendMessage = function(recipient, messageHead, messageBody)
{
	const self = this;

	const lockFileName = "lock_" + self.selfName + IPC.LockFileCount.toString();
	WriteStringToFile(IpcPath + "/" + lockFileName, "");
	++IPC.LockFileCount;

	const fileNameVec = ListFilesInDirectory(IpcPath);

	var found = true;
	var fileCount = 0;
	while (found)
	{
		found = false;
		for (var fileNameVecIndex in fileNameVec)
		{
			const fileNameCurrent = fileNameVec[fileNameVecIndex];
			if (fileNameCurrent == recipient + fileCount.toString())
			{
				found = true;
				++fileCount;
				break;
			}
		}
	}

	const pathNew = IpcPath + "/" + recipient + fileCount.toString();
	WriteStringToFile(pathNew, messageHead + "!" + messageBody);

	++IPC.SentCount;
	DeleteFile(IpcPath + "/" + lockFileName);

	//console.log("message sent: " + recipient + " " + messageHead + " " + messageBody);
};

IPC.prototype.GetResponse = function(recipient, messageHead, messageBody, callback)
{
	const self = this;

	self.SendMessage(recipient, messageHead, messageBody);
	self.responseMap[messageHead] = callback;
};

IPC.prototype.MapFunction = function(messageHead, callback)
{
	const self = this;

	self.commandMap[messageHead] = callback;
};

IPC.prototype.SetUDPCallback = function(udpCallbackIn)
{
	const self = this;

	self.udp.SetUDPCallback(udpCallbackIn);
};