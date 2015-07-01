var S3 = function()
{
	var self = this;

	self.awsAccessKeyID = "";
	self.awsSecretAccessKey = "";
	self.awsBucketName = "";

	AWS.config.update({ accessKeyId: self.awsAccessKeyID, secretAccessKey: self.awsSecretAccessKey });
};

S3.prototype.GetKeys = function(callback, errorCallback)
{
	var self = this;

	new AWS.S3().listObjects({ Bucket: self.awsBucketName }, function(error, data)
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

	new AWS.S3().getObject({ Bucket: self.awsBucketName, Key: keyName }, function(err, data)
	{
		if (!err && callback != null && callback != undefined)
			callback(data.Body.toString());
		else if (err && errorCallback != null && errorCallback != undefined)
			errorCallback();
	});
};

S3.prototype.WriteTextKey = function(keyName, keyString, callback)
{
	var self = this;

	var s3obj = new AWS.S3({ params: { Bucket: self.awsBucketName, Key: keyName } });
	s3obj.upload({ Body: keyString }, function()
	{
		if (callback != null && callback != undefined)
			callback();
    });
}

S3.prototype.DownloadKey = function(keyName, path, callback, errorCallback)
{
	var self = this;

	new AWS.S3().getObject({ Bucket: self.awsBucketName, Key: keyName }, function(err, data)
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
};