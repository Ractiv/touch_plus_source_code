package
{
	import flash.display.*;
	import flash.utils.*;
	import flash.external.*;
	
	public class DebugPage extends MovieClip
	{
		private var reading:Boolean = false;
		private var lines_array_old:Array = new Array();

		public function DebugPage():void
		{
			progress_fan0.inner = false;
			progress_fan1.inner = true;

			setInterval(function():void
			{
				if (Globals.active_name != "ButtonDebug")
					return;

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
								
								console_text.appendText(lines_array[i] + "\n");
							}
							
							console_text.scrollV = console_text.maxScrollV;
							lines_array_old = lines_array;
						}
						reading = false;
					});
				}
				
				Interop.call_js("get_var(status)", function(obj):void
				{
					var status:String = obj as String;
					status_text.text = status;
				});
				
				Interop.call_js("get_var(loading_percent)", function(obj):void
				{
					var loading_percent:int = obj as int;
					progress_fan0.progress_percent = loading_percent;
				});
				
				Interop.call_js("get_var(downloading_percent)", function(obj):void
				{
					var downloading_percent:int = obj as int;
					progress_fan1.progress_percent = downloading_percent;
				});
				
			}, 500);
		}
	}
}