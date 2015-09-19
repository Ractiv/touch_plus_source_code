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
	import flash.geom.*;
	import flash.external.*;

	public class ScrollBar extends MovieClip
	{
		private var self = this;

		private var levels:int = 9;
		private var level_max:int = levels / 2;
		private var level_min:int = -(levels - level_max);

		public var level_current:int = 0;
		public var level_current_raw:int = 0;
		private var mouse_down:Boolean = false;

		public function ScrollBar():void
		{
			set_circle_x(scroll_bar_line.width / 2 + scroll_bar_line.x);

			scroll_bar_button.addEventListener(MouseEvent.MOUSE_DOWN, function(e:MouseEvent):void
			{
				mouse_down = true;
			});

			scroll_bar_button.addEventListener(MouseEvent.MOUSE_UP, function(e:MouseEvent):void
			{
				mouse_down = false;
			});

			scroll_bar_button.addEventListener(MouseEvent.RELEASE_OUTSIDE, function(e:MouseEvent):void
			{
				mouse_down = false;
			});

			var mouse_down_old:Boolean = false;
			self.addEventListener(Event.ENTER_FRAME, function(e:Event)
			{
				if (Globals.active_name != "ButtonSettings")
					return;

				if (mouse_down)
					set_circle_x(scroll_bar_line.mouseX);

				if (!mouse_down && mouse_down_old)
					ExternalInterface.call("scroll_bar_moved", self.parent.name, level_current_raw);
				
				mouse_down_old = mouse_down;
			});
		}

		private function set_circle_x(x_val:int):void
		{
			var segment_length:int = scroll_bar_line.width / levels;

			var x_diff:Number = 0;
			var x_diff_min:Number = 9999;
			var i_x_diff_min:int = 0;
			var index_x_diff_min:int = 0;

			var index:int = 0;
			var i_max:int = scroll_bar_line.x + scroll_bar_line.width;
			for (var i:int = scroll_bar_line.x; i <= i_max; i += segment_length)
			{
				x_diff = Math.abs(i + 5 - x_val);
				if (x_diff < x_diff_min)
				{
					x_diff_min = x_diff;
					i_x_diff_min = i + 5;
					index_x_diff_min = index;
				}
				++index;
			}
			scroll_bar_circle.x = int(i_x_diff_min);
			level_current = index_x_diff_min - (levels / 2);
			level_current_raw = index_x_diff_min;
		}

		public function set_level(val:int):void
		{
			level_current = val;
			scroll_bar_circle.x = int(Globals.map_val(val, level_min, level_max,
				scroll_bar_line.x + 4, scroll_bar_line.x + scroll_bar_line.width + 4));
		}

		public function set_level2(val:int):void
		{
			level_current = val;
			scroll_bar_circle.x = int(Globals.map_val(val, 0, levels,
				scroll_bar_line.x + 4, scroll_bar_line.x + scroll_bar_line.width + 4));
		}
	}
}
