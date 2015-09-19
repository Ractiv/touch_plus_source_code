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
	public class Globals
	{
		private var self = this;
		
		public static var stage_width:int = 640;
		public static var stage_height:int = 675;
		public static var active_name:String;

		public static var text_bubble:TextBubble = new TextBubble();
		public static var menu_bar:MenuBar;

		public static function map_val(value:Number, left_min:Number, left_max:Number, right_min:Number, right_max:Number):Number
		{
		    var left_span:Number = left_max - left_min;
		    var right_span:Number = right_max - right_min;
		    var value_scaled:Number = (value - left_min) / left_span;
		    return right_min + (value_scaled * right_span);
		}
	}
}