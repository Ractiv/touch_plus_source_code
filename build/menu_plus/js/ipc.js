var IPC = function(selfNameIn)
{
	var self = this;

	self.selfName = selfNameIn;

	self.MapFunction("open udp channel", function(messageBody)
	{
		var port = self.udp.Assign();
		self.SendMessage("track_plus", "open udp channel", port.toString());
	});
};

IPC.Updated = 0;
IPC.FileNameProcessedMap = new Array();
IPC.SentCount = 0;

IPC.prototype.selfName = "";
IPC.prototype.responseMap = new Array();
IPC.prototype.commandMap = new Array();
IPC.prototype.udp = new UDP();

IPC.prototype.Update = function()
{
	var self = this;

	if (IPC.Updated > 0)
		return;

	var fileNameVec = ListFilesInDirectory(IpcPath);
	for (var fileNameVecIndex in fileNameVec)
	{
		var fileNameCurrent = fileNameVec[fileNameVecIndex];

		if (fileNameCurrent.length > self.selfName.length)
		{
			if (IPC.FileNameProcessedMap[fileNameCurrent] == true)
				continue;
			else
				IPC.FileNameProcessedMap[fileNameCurrent] = true;

			var fileName = fileNameCurrent.substring(0, self.selfName.length);
			var fileNameID = fileNameCurrent.substring(self.selfName.length, fileNameCurrent.length);
			
			if (fileName == self.selfName)
			{
				++IPC.Updated;

				var lines = ReadTextFile(IpcPath + "/" + fileNameCurrent);
				// DeleteFile(IpcPath + "/" + fileNameCurrent);

				var messageVec = lines[0].split("!");
				var messageHead = messageVec[0];
				var messageBody = messageVec[1];

				console.log("message received " + messageHead + " " + messageBody + " " + fileNameCurrent + " " + fileName);

				if (self.responseMap[messageHead] == null)
				{
					if (self.commandMap[messageHead] != null)
						self.commandMap[messageHead](messageBody);
				}
				else
				{
					var func = self.responseMap[messageHead];
					self.responseMap[messageHead] = null;
					func(messageBody);
				}

				--IPC.Updated;

			}
		}
	}
};

IPC.prototype.SendMessage = function(recipient, messageHead, messageBody)
{
	var self = this;

	var fileNameVec = ListFilesInDirectory(IpcPath);

	var found = true;
	var fileCount = 0;

	while (found)
	{
		found = false;
		for (var fileNameVecIndex in fileNameVec)
		{
			var fileNameCurrent = fileNameVec[fileNameVecIndex];

			if (fileNameCurrent == recipient + fileCount.toString())
			{
				found = true;
				++fileCount;
				break;
			}
		}
	}

	var pathOld = IpcPath + "/s" + self.selfName + IPC.SentCount.toString();
	var pathNew = IpcPath + "/" + recipient + fileCount.toString();

	WriteStringToFile(pathOld, messageHead + "!" + messageBody);
	Renamefile(pathOld, pathNew);

	++IPC.SentCount;

	console.log("message sent: " + recipient + " " + messageHead + " " + messageBody);
};

IPC.prototype.GetResponse = function(recipient, messageHead, messageBody, callback)
{
	var self = this;

	self.SendMessage(recipient, messageHead, messageBody);
	self.responseMap[messageHead] = callback;
};

IPC.prototype.MapFunction = function(messageHead, callback)
{
	var self = this;

	self.commandMap[messageHead] = callback;
};

IPC.prototype.SetUDPCallback = function(udpCallbackIn)
{
	var self = this;

	self.udp.SetUDPCallback(udpCallbackIn);
};