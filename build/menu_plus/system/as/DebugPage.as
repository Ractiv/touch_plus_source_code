package
{
	import flash.display.*;
	import flash.utils.*;
	import flash.external.*;
	import flash.events.*;
	
	public class DebugPage extends MovieClip
	{
		private var self = this;
		private var reading:Boolean = false;
		private var lines_array_old:Array = new Array();
		private var loaded:Boolean = false;
		private var loaded_old:Boolean = false;
		private var target_alpha:int = 0;

		public function DebugPage():void
		{
			progress_fan0.inner = false;
			progress_fan1.inner = true;

			progress_fan0.alpha = 0;
			progress_fan1.alpha = 0;
			status_text.alpha = 0;

			var active_name_old:String = "";

			self.addEventListener(Event.ENTER_FRAME, function(e:Event):void
			{
				if (Globals.active_name == "ButtonDebug" && active_name_old != Globals.active_name)
					read_log();

				active_name_old = Globals.active_name;

				if (Globals.active_name != "ButtonDebug")
					return;

				if (progress_fan0.alpha < 0.1 && loaded && progress_fan0.visible)
				{
					progress_fan0.visible = false;
					progress_fan1.visible = false;
					status_text.visible = false;
					Globals.menu_bar.activate_index(3);
				}
				else
				{
					loaded = progress_fan1.progress_percent_current >= 99 &&
						 progress_fan0.progress_percent_current >= 99 &&
						 progress_fan1.progress_percent == 100 &&
						 progress_fan0.progress_percent == 100;

					if (loaded && !loaded_old)
						hide_progress();

					loaded_old = loaded;

					var current_alpha:Number = progress_fan0.alpha;
					var alpha_diff:Number = (target_alpha - current_alpha) / 10;
					progress_fan0.alpha += alpha_diff;
					progress_fan1.alpha += alpha_diff;
					status_text.alpha += alpha_diff;
				}
			});

			setInterval(function():void
			{
				if (Globals.active_name != "ButtonDebug")
					return;

				read_log();

			}, 1000);
		}

		public function read_log():void
		{
			if (!reading)
			{
				reading = true;
				Interop.call_js("ReadTextFile('c:/touch_plus_software_log.txt')", function(obj):void
				{
					var lines_array:Array = obj as Array;
					if (lines_array.length > lines_array_old.length)
					{
						for (var i:int = lines_array_old.length - 1; i < lines_array.length; ++i)
						{
							if (i == -1)
								continue;
							
							if (lines_array[i] != "")
								console_text.appendText(lines_array[i] + "\n");
						}
						
						console_text.scrollV = console_text.maxScrollV;
						lines_array_old = lines_array;
					}
					reading = false;
				});
			}
		}

		public function show_progress():void
		{
			target_alpha = 1;
		}

		public function hide_progress():void
		{
			target_alpha = 0;
		}

		public function set_downloading_progress(percent:int):void
		{
			if (percent <= progress_fan1.progress_percent)
				return;

			progress_fan1.progress_percent = percent;
			show_progress();
		}

		public function set_loading_progress(percent:int):void
		{
			if (percent <= progress_fan0.progress_percent)
				return;

			progress_fan0.progress_percent = percent;
			show_progress();
		}

		public function set_status(_status:String):void
		{
			status_text.text = _status;
		}
	}
}