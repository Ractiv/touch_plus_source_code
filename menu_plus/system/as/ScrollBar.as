package
{
	import flash.display.*;
	import flash.events.*;
	import flash.geom.*;

	public class ScrollBar extends MovieClip
	{
		private var self = this;

		private var levels:int = 10;
		private var level_max:int = levels / 2;
		private var level_min:int = -(levels - level_max);

		public var level_current:int = 0;
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

			self.addEventListener(Event.ENTER_FRAME, function(e:Event)
			{
				if (Globals.active_name != "ButtonSettings")
					return;

				if (mouse_down)
					set_circle_x(scroll_bar_line.mouseX);
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
			scroll_bar_circle.x = i_x_diff_min;
			level_current = index_x_diff_min - (levels / 2);
		}

		public function set_level(val:int):void
		{
			level_current = val;
			scroll_bar_circle.x = Globals.map_val(val, level_min, level_max,
				scroll_bar_line.x + 4, scroll_bar_line.x + scroll_bar_line.width + 4);
		}
	}
}
