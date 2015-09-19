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
	import flash.external.*;
	
	public class Toggle extends MovieClip
	{
		private var self = this;

		private var inner_x_left:int = 25;
		private var inner_x_right:int = 52;

		public var is_on:Boolean = false;

		public function Toggle():void
		{
			self.addEventListener(MouseEvent.CLICK, function(e:MouseEvent):void
			{
				ExternalInterface.call("toggle_clicked", self.parent.name, is_on);
				// if (is_on)
				// 	is_on = false;
				// else
				// 	is_on = true;
			});

			self.addEventListener(Event.ENTER_FRAME, function()
			{
				if (Globals.active_name != "ButtonSettings")
					return;

				var inner_dest_x:int = is_on ? inner_x_right : inner_x_left;

				var x_diff:Number = (inner_dest_x - inner.x) / 5;
				if (x_diff > 3)
					x_diff = 3;
				else if (x_diff < -3)
					x_diff = -3;

				inner.x += x_diff;
			});
		}

		public function switch_toggle(_is_on:Boolean):void
		{
			is_on = _is_on;
		}
	}
}
