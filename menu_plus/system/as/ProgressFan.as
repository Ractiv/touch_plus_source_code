package
{
	import flash.display.*;
	import flash.geom.*;
	import flash.events.*;
	
	public class ProgressFan extends MovieClip
	{
		private const self = this;
		
		public var progress_percent:Number = 0;
		public var progress_percent_current:Number = 0;
		public var inner:Boolean = false;
		
		public function ProgressFan():void
		{
			const position_marker_x:Number = self.progress_fan_half0.position_marker.x;
			self.progress_fan_frame.visible = false;

			self.addEventListener(Event.ENTER_FRAME, function(e:Event):void
			{
				if (Globals.active_name != "ButtonDebug")
					return;

				if (self.inner)
				{
					if (self.progress_percent_current < self.progress_percent)
						self.progress_percent_current += 0.2;
					else
						self.progress_percent_current = self.progress_percent;

					self.progress_fan_half0.position_marker.x = position_marker_x - 20;
					self.progress_fan_half1.position_marker.x = position_marker_x - 20;
				}
				else
				{
					if (self.progress_percent_current < self.progress_percent)
						self.progress_percent_current += 0.2;
					else
						self.progress_percent_current = self.progress_percent;

					self.progress_fan_half0.position_marker.x = position_marker_x + 20;
					self.progress_fan_half1.position_marker.x = position_marker_x + 20;
				}

				var progress_fan_text_pos_global:Point;
				var progress_fan_text_pos_local:Point;

				if (self.progress_percent_current < 50)
				{
					progress_fan_text_pos_global = self.progress_fan_half0.localToGlobal
					(
						new Point(self.progress_fan_half0.position_marker.x, self.progress_fan_half0.position_marker.y)
					);

					progress_fan_text_pos_local = self.globalToLocal(new Point(progress_fan_text_pos_global.x, progress_fan_text_pos_global.y));

					self.progress_fan_half0.rotation = Globals.map_val(self.progress_percent_current, 0, 50, 180, 360);
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

					self.progress_fan_half1.rotation = Globals.map_val(self.progress_percent_current, 50, 100, 0, 180);
					self.progress_fan_half0.rotation = 0;
					self.progress_fan_half1.visible = true;
				}

				self.progress_fan_text.x = progress_fan_text_pos_local.x;
				self.progress_fan_text.y = progress_fan_text_pos_local.y;
				self.progress_fan_text.progress_text.text = self.progress_percent_current.toString();
			});
		}
	}
}
