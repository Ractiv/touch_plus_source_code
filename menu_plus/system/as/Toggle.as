package
{
	import flash.display.*;
	import flash.events.*;
	
	public class Toggle extends MovieClip
	{
		private const self = this;

		private const inner_x_left:int = 25;
		private const inner_x_right:int = 52;

		public var is_on:Boolean = false;

		public function Toggle():void
		{
			self.addEventListener(MouseEvent.CLICK, function(e:MouseEvent):void
			{
				if (is_on)
					is_on = false;
				else
					is_on = true;
			});

			self.addEventListener(Event.ENTER_FRAME, function()
			{
				if (Globals.active_name != "ButtonSettings")
					return;

				const inner_dest_x:int = is_on ? inner_x_right : inner_x_left;

				var x_diff:Number = (inner_dest_x - inner.x) / 5;
				if (x_diff > 3)
					x_diff = 3;
				else if (x_diff < -3)
					x_diff = -3;

				inner.x += x_diff;
			});
		}

		public function switch_toggle():void
		{
			if (is_on)
				is_on = false;
			else
				is_on = true;
		}
	}
}
