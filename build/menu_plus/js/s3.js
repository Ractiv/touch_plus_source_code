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

require("nw.gui").Window.get().evalNWBin(null, ExecutablePath + "/menu_plus/js/aws-credentials.bin");

// var AwsAccessKeyID = "";
// var AwsSecretAccessKey = "";
// var AwsBucketName = "";

var S3 = function()
{
	var self = this;
	AWS.config.update({ accessKeyId: AwsAccessKeyID, secretAccessKey: AwsSecretAccessKey });
};

S3.prototype.GetKeys = function(callback, errorCallback)
{
	var self = this;

	new AWS.S3().listObjects({ Bucket: AwsBucketName }, function(error, data)
	{
		if (!error && callback != null && callback != undefined)
			callback(data.Contents);
		else if (error && errorCallback != null && errorCallback != undefined)
			errorCallback();
	});	
};

S3.prototype.ReadTextKey = function(keyName, callback, errorCallback)
{
	var self = this;

	var request = new AWS.S3().getObject({ Bucket: AwsBucketName, Key: keyName }, function(err, data)
	{
		if (!err && callback != null && callback != undefined)
		{
			//console.log("text key read: " + keyName);
			callback(data.Body.toString());
		}
		else if (err && errorCallback != null && errorCallback != undefined)
		{
			//console.log("read text key failed: " + keyName);
			errorCallback();
		}
	});

	request.on("httpDownloadProgress", function(progress)
	{
		//console.log(progress.loaded + " of " + progress.total + " bytes");
	});
};

S3.prototype.WriteTextKey = function(keyName, keyString, callback)
{
	var self = this;

	var s3obj = new AWS.S3({ params: { Bucket: AwsBucketName, Key: keyName } });
	s3obj.upload({ Body: keyString }, function()
	{
		if (callback != null && callback != undefined)
			callback();
    });
}

S3.prototype.UploadKey = function(keyName, path, callback)
{
	var self = this;

	fs.readFile(path, function(err, data)
	{
		if (err) { throw err; }

		var base64data = new Buffer(data, "binary");

		var s3obj = new AWS.S3({ params: { Bucket: AwsBucketName, Key: keyName } });
		s3obj.upload({ Body: base64data }, function()
		{
			if (callback != null && callback != undefined)
				callback();
	    });
	});
}

S3.prototype.DownloadKey = function(keyName, path, callback, errorCallback, progressCallback)
{
	var self = this;

	var request = new AWS.S3().getObject({ Bucket: AwsBucketName, Key: keyName }, function(err, data)
	{
		if (!err)
		{
			var buffer = new Buffer(data.Body.length);
			for (var i = 0; i < data.Body.length; ++i)
			    buffer.writeUInt8(data.Body[i], i);

			var fileNameSplit = keyName.split("/");
			var fileName = fileNameSplit[fileNameSplit.length - 1];

			fs.writeFileSync(path + "/" + fileName, buffer);

			if (callback != null && callback != undefined)
			callback(path + "/" + fileName);
		}
		else if (err && errorCallback != null && errorCallback != undefined)
			errorCallback();
	});

	request.on("httpDownloadProgress", function(progress)
	{
		//console.log(progress.loaded + " of " + progress.total + " bytes");
		progressCallback(progress.loaded, progress.total);
	});
};