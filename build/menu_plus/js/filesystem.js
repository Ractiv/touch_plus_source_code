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