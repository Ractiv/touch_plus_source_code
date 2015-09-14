package
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

			var debug_page:DebugPage = new DebugPage();
			add_page(debug_page);

			var show_menu_bar_shade:Boolean = false;
			var menu_bar_shade:MenuBarShade = new MenuBarShade();
			menu_bar_shade.x = 0;
			menu_bar_shade.y = 0;
			menu_bar_shade.alpha = 0;
			self.addChild(menu_bar_shade);

			var menu_bar:MenuBar = new MenuBar();
			self.addChild(menu_bar);

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
				if (menu_bar.active_name == "ButtonDebug" || menu_bar.active_name == "ButtonVisualize")
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
