package
{
	import flash.display.*;
	
	public class GraphicsLayer extends MovieClip
	{
		var self:MovieClip;

		public function GraphicsLayer():void
		{
			self = this;
		}

		public function line(x0:int, y0:int, x1:int, y1:int, color:Number, thickness:int):void
		{
	        self.graphics.lineStyle(thickness, color);
			self.graphics.moveTo(x0, y0);
			self.graphics.lineTo(x1, y1);
		}

		public function circle(x, y, radius, color, fill_color, thickness):void
		{
			if (thickness == -1)
			{
				self.graphics.beginFill(fill_color, 1);
				self.graphics.lineStyle(thickness, color);
				self.graphics.drawCircle(x, y, radius);
				self.graphics.endFill();
			}
			else
			{
				self.graphics.lineStyle(thickness, color);
				self.graphics.drawCircle(x, y, radius);
			}
		}

		public function clear()
		{
			self.graphics.clear();
		}
	}
}
