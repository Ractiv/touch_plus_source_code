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

﻿package
{
	import flash.display.*;
	import flash.external.*;
	import flash.events.*;
	
	public class SupportPage extends MovieClip
	{
		public function SupportPage():void
		{
			button_send.addEventListener(MouseEvent.CLICK, function(e:MouseEvent):void
			{
				if (text_email_address.text.length > 5 &&
					text_message.text.length > 5 &&
					text_email_address.text.indexOf("@") >= 0 &&
					text_email_address.text.indexOf(".") >= 0 &&
					text_email_address.text.indexOf(" ") < 0)
				{
					ExternalInterface.call("send_feedback", text_email_address.text, text_message.text);
					Globals.text_bubble.show_text("sending...", 0);
				}
				else
					Globals.text_bubble.show_text("please enter a valid email address and message", 2000);
			});
		}

		public function message_sent():void
		{
			text_email_address.text = "";
			text_message.text = "";
			Globals.text_bubble.show_text("message sent", 2000);
		}

		public function message_failed():void
		{
			Globals.text_bubble.show_text("server unreachable, please check your internet connection", 2000);
		}
	}
}
