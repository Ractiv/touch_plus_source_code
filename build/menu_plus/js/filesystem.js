var fs = require("fs");

function ListFilesInDirectory(path)
{
	var fileNameVec = fs.readdirSync(path);
	return fileNameVec;   
}

function FileExists(path)
{
	try
	{
	    stats = fs.lstatSync(path);
	    if (stats.isFile())
	    	return true;
	}
	catch (e) { }

	return false;
}

function WriteStringToFile(path, str)
{
	fs.writeFileSync(path, str);
}

function ReadTextFile(path)
{
	var str = fs.readFileSync(path, "utf8");
	var lines = str.split(/[\u000d\u000a\u0008]+/g);

	var iMax = lines.length;
	for (var i = 0; i < iMax; ++i)
		lines[i] = lines[i].replace(/[\u000d\u000a\u0008]+/g, "");
	
	return lines;
}

function DeleteFile(path)
{
	fs.unlinkSync(path);
}

function Renamefile(pathOld, pathNew)
{
	fs.rename(pathOld, pathNew);
}