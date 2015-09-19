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
	
	public class ButtonSimple extends MovieClip
	{
		private var self = this;
		private var default_width:Number = 125.35;

		[Inspectable(name = "label", type = String)]
		public function set label(val:String):void
		{
			button_simple_text.label_text.text = val;
			rescale();
		}
		public function get label():String
		{
           return button_simple_text.label_text.text;
      	}

      	private function rescale():void
		{
			var width_scale = self.width / default_width;
			button_simple_text.width /= width_scale;
			button_simple_text.label_text.width = button_simple_text.label_text.textWidth + 5;
			button_simple_text.x = (default_width / 2) - (button_simple_text.width / 2);
		}

		public function ButtonSimple():void
		{
		}
	}
}
