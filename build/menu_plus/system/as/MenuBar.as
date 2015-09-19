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
	import flash.utils.*;
	
	public class MenuBar extends MovieClip
	{
		private var self = this;

		private var area_width:int = Globals.stage_width;
		private var area_height:int = 100;
		private var spacing:int = 50;
		private var button_width:int = 50;
		private var button_height:int = 50;

		private var button_array:Array = new Array();

		public var active_index:int = 0;
		public var active_name:String;

		public function init():void
		{
			button_array.push(new ButtonSettings());
			button_array.push(new ButtonVisualize());
			button_array.push(new ButtonSupport());
			button_array.push(new ButtonTutorial());
			button_array.push(new ButtonDebug());

			if (button_array.length == 0)
				return;

			var total_width:int = (button_array.length * button_width) + ((button_array.length - 1) * spacing);
			var x_min:int = (area_width / 2) - (total_width / 2);
			var y_min:int = (area_height / 2) - (button_height / 2);

			var x_current:Number = x_min;
			for (var i = 0; i < button_array.length; ++i)
			{
				var button_current = button_array[i];
				button_current.x = x_current;
				button_current.y = y_min;

				self.addChild(button_current);
				x_current += button_width + spacing;
				button_current.addEventListener(MouseEvent.CLICK, on_click(button_current));
			}

			activate_index(0);
		}

		public function activate_index(index:int):void
		{
			activate(button_array[index]);
		}

		private function on_click(button):Function
		{
			return function(e:MouseEvent):void
			{
				activate(button);
			}
		}

		private function activate(button)
		{
			var new_color:ColorTransform = new ColorTransform();
			for (var i:int = 0; i < button_array.length; ++i)
			{
				var button_current = button_array[i];
				if (button_current != button)
				{
					new_color.color = 0x999999;
					button_current.transform.colorTransform = new_color;
				}
				else
				{
					active_index = i;
					active_name = getQualifiedClassName(button);
					Globals.active_name = active_name;
				}
			}
			new_color.color = 0x666666;
			button.transform.colorTransform = new_color;
		}
	}
}