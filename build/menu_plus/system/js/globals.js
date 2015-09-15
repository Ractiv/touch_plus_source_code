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

var ExecutablePath = require("path").join(process.cwd(), "..");
var IpcPath = ExecutablePath + "/ipc";
var UserDataPath = ExecutablePath + "/userdata";
var BlockExit = false;

function show_notification(message_head, message_body, cb)
{
	var notification = new Notification(message_head, { body: message_body, icon: "file://" + process.cwd() + "system/images/ractiv.png" });
	if (typeof cb !== "undefined")
		notification.onclick = cb;
	else
		notification.onclick = function(){};
}