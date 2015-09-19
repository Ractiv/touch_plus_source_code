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
	import flash.events.*;
	import flash.utils.*;

	public class TextBubble extends MovieClip
	{
		private var self = this;
		private var content_old:String = "";
		private var do_play:Boolean = false;
		private var do_timeout:Boolean = false;
		private var timeout_ms:int = 0;

		public function init():void
		{
			self.mouseEnabled = false;
			self.mouseChildren = false;

			self.addEventListener(Event.ENTER_FRAME, function(e:Event):void
			{
				if (do_play)
				{
					do_play = false;
					text_bubble_mask.play();
				}
			});

			stage.addEventListener(MouseEvent.MOUSE_MOVE, function(e:MouseEvent):void
			{
				if (do_timeout)
				{
					do_timeout = false;
					setTimeout(function():void
					{
						do_play = true;

					}, timeout_ms);
				}
			});
		}

		public function show_text(content:String, ms:int):void
		{
			text_message.text = content;
			text_background.width = text_message.textWidth + 5;
			text_background.height = text_message.textHeight + 5;

			if (content_old != content || text_bubble_mask.currentFrame == 1)
			{
				text_bubble_mask.gotoAndPlay(2);

				if (ms != 0)
				{
					timeout_ms = ms;
					do_timeout = true;
				}
			}
				
			content_old = content;
		}
	}
}
