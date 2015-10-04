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
	import flash.geom.*;
	
	public class Main extends MovieClip
	{
		private var self = this;

		private var page_count = 0;
		private var page_array:Array = new Array();
		private var page_container:Sprite = new Sprite();

		public function Main():void
		{
			stage.addEventListener(MouseEvent.RIGHT_CLICK, function(e:MouseEvent):void{});

			page_container.x = 0;
			page_container.y = 0;
			self.addChild(page_container);

			var settings_page:SettingsPage = new SettingsPage();
			add_page(settings_page);

			var visualize_page:VisualizePage = new VisualizePage();
			add_page(visualize_page);

			var support_page:SupportPage = new SupportPage();
			add_page(support_page);

			var tutorial_page:TutorialPage = new TutorialPage();
			add_page(tutorial_page);

			var debug_page:DebugPage = new DebugPage();
			add_page(debug_page);

			var show_menu_bar_shade:Boolean = false;
			var menu_bar_shade:MenuBarShade = new MenuBarShade();
			menu_bar_shade.x = 0;
			menu_bar_shade.y = 0;
			menu_bar_shade.alpha = 0;
			self.addChild(menu_bar_shade);

			Globals.menu_bar = new MenuBar();
			var menu_bar:MenuBar = Globals.menu_bar;
			self.addChild(menu_bar);
			menu_bar.init();

			var text_bubble:TextBubble = Globals.text_bubble;
			self.addChild(text_bubble);
			text_bubble.init();
			text_bubble.startDrag(true);

			var x_dest_old:int = 0;
			var speed_cap:Number = 9999;
			self.addEventListener(Event.ENTER_FRAME, function(e:Event):void
			{
				var x_dest:int = -menu_bar.active_index * Globals.stage_width;
				if (x_dest != x_dest_old)
					speed_cap = Math.abs(x_dest - page_container.x) / 20;

				x_dest_old = x_dest;

				var x_diff:Number = (x_dest - page_container.x) / 5;
				if (x_diff > speed_cap)
					x_diff = speed_cap;
				else if (x_diff < -speed_cap)
					x_diff = -speed_cap;

				page_container.x += x_diff;
				if (Math.abs(x_diff) < 0.05)
					page_container.x = x_dest;

				for (var i:int = 0; i < page_array.length; ++i)
				{
					var page_current = page_array[i];
					var global_pos:Point = page_container.localToGlobal(new Point(page_current.x, page_current.y));

					if (global_pos.x <= -Globals.stage_width || global_pos.x >= Globals.stage_width)
						page_current.visible = false;
					else
						page_current.visible = true;
				}

				show_menu_bar_shade = false;
				if (menu_bar.active_name == "ButtonDebug" ||
					menu_bar.active_name == "ButtonVisualize" ||
					menu_bar.active_name == "ButtonTutorial")
						show_menu_bar_shade = true;

				var menu_bar_shade_alpha:int = show_menu_bar_shade ? 1 : 0;
				var alpha_diff:Number = (menu_bar_shade_alpha - menu_bar_shade.alpha) / 10;
				menu_bar_shade.alpha += alpha_diff;
			});

			ExternalInterface.addCallback("switch_toggle", function(toggle_name:String, _is_on:Boolean):void
			{
				var settings_item:SettingsItem = settings_page.getChildByName(toggle_name) as SettingsItem;
				settings_item.toggle.switch_toggle(_is_on);
			});

			ExternalInterface.addCallback("set_scroll_bar", function(scroll_bar_name:String, _level:int):void
			{
				var settings_item:SettingsItem = settings_page.getChildByName(scroll_bar_name) as SettingsItem;
				settings_item.scroll_bar.set_level2(_level);
			});

			ExternalInterface.addCallback("message_sent", support_page.message_sent);
			ExternalInterface.addCallback("message_failed", support_page.message_failed);

			ExternalInterface.addCallback("show_page", function(index:int):void
			{
				menu_bar.activate_index(index);
			});

			ExternalInterface.addCallback("reset_progress", debug_page.reset_progress);
			ExternalInterface.addCallback("set_downloading_progress", debug_page.set_downloading_progress);
			ExternalInterface.addCallback("set_loading_progress", debug_page.set_loading_progress);
			ExternalInterface.addCallback("set_status", debug_page.set_status);
			ExternalInterface.addCallback("error_screen_on", debug_page.error_screen_on);
			ExternalInterface.addCallback("error_screen_off", debug_page.error_screen_off);
			ExternalInterface.addCallback("console_log", debug_page.console_log);

			ExternalInterface.call("set_gui_ready");
		}

		private function add_page(page):void
		{
			page_container.addChild(page);
			page.y = 0;
			page.x = page_count * Globals.stage_width;
			++page_count;
			page_array.push(page);
		}
	}
}
