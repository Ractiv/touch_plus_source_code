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
	import flash.geom.*;
	import flash.events.*;
	
	public class ProgressFan extends MovieClip
	{
		private var self = this;
		
		public var progress_percent:Number = 0;
		public var progress_percent_current:Number = 0;
		public var inner:Boolean = false;
		
		public function ProgressFan():void
		{
			var position_marker_x:Number = self.progress_fan_half0.position_marker.x;
			self.progress_fan_frame.visible = false;

			self.addEventListener(Event.ENTER_FRAME, function(e:Event):void
			{
				if (Globals.active_name != "ButtonDebug")
					return;

				if (progress_percent_current < progress_percent)
				{
					var percent_diff = (progress_percent - progress_percent_current) / 20;
					progress_percent_current += percent_diff;
				}
				else
					progress_percent_current = progress_percent;

				if (inner)
				{
					self.progress_fan_half0.position_marker.x = position_marker_x - 20;
					self.progress_fan_half1.position_marker.x = position_marker_x - 20;
				}
				else
				{
					self.progress_fan_half0.position_marker.x = position_marker_x + 20;
					self.progress_fan_half1.position_marker.x = position_marker_x + 20;
				}

				var progress_fan_text_pos_global:Point;
				var progress_fan_text_pos_local:Point;

				if (progress_percent_current < 50)
				{
					progress_fan_text_pos_global = self.progress_fan_half0.localToGlobal
					(
						new Point(self.progress_fan_half0.position_marker.x, self.progress_fan_half0.position_marker.y)
					);

					progress_fan_text_pos_local = self.globalToLocal(new Point(progress_fan_text_pos_global.x, progress_fan_text_pos_global.y));

					self.progress_fan_half0.rotation = Globals.map_val(progress_percent_current, 0, 50, 180, 360);
					self.progress_fan_half1.rotation = 0;
					self.progress_fan_half1.visible = false;
				}
				else
				{
					progress_fan_text_pos_global = self.progress_fan_half1.localToGlobal
					(
						new Point(self.progress_fan_half1.position_marker.x, self.progress_fan_half1.position_marker.y)
					);

					progress_fan_text_pos_local = self.globalToLocal(new Point(progress_fan_text_pos_global.x, progress_fan_text_pos_global.y));

					self.progress_fan_half1.rotation = Globals.map_val(progress_percent_current, 50, 100, 0, 180);
					self.progress_fan_half0.rotation = 0;
					self.progress_fan_half1.visible = true;
				}

				self.progress_fan_text.x = progress_fan_text_pos_local.x;
				self.progress_fan_text.y = progress_fan_text_pos_local.y;
				self.progress_fan_text.progress_text.text = progress_percent_current.toString();
			});
		}
	}
}
