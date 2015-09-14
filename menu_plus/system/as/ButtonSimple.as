package
{
	import flash.display.*;
	import flash.events.*;
	
	public class ButtonSimple extends MovieClip
	{
		private const self = this;
		private const default_width:Number = 125.35;

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
			const width_scale = self.width / default_width;
			button_simple_text.width /= width_scale;
			button_simple_text.label_text.width = button_simple_text.label_text.textWidth + 5;
			button_simple_text.x = (default_width / 2) - (button_simple_text.width / 2);
		}

		public function ButtonSimple():void
		{
		}
	}
}
