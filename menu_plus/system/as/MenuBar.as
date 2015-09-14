package
{
	import flash.display.*;
	import flash.events.*;
	import flash.geom.*;
	import flash.utils.*;
	
	public class MenuBar extends MovieClip
	{
		private const self = this;

		private const area_width:int = Globals.stage_width;
		private const area_height:int = 100;
		private const spacing:int = 50;
		private const button_width:int = 50;
		private const button_height:int = 50;

		private const button_array:Array = new Array();

		public var active_index:int = 0;
		public var active_name:String;

		public function MenuBar():void
		{
			const button_settings:ButtonSettings = new ButtonSettings();
			const button_support:ButtonSupport = new ButtonSupport();
			const button_visualize:ButtonVisualize = new ButtonVisualize();
			const button_dev_tool:ButtonDebug = new ButtonDebug();

			button_array.push(button_settings);
			button_array.push(button_visualize);
			button_array.push(button_support);
			button_array.push(button_dev_tool);

			if (button_array.length == 0)
				return;

			const total_width:int = (button_array.length * button_width) + ((button_array.length - 1) * spacing);
			const x_min:int = (area_width / 2) - (total_width / 2);
			const y_min:int = (area_height / 2) - (button_height / 2);

			var x_current:Number = x_min;
			for (var i = 0; i < button_array.length; ++i)
			{
				const button_current = button_array[i];
				button_current.x = x_current;
				button_current.y = y_min;

				self.addChild(button_current);
				x_current += button_width + spacing;
				button_current.addEventListener(MouseEvent.CLICK, on_click(button_current));
			}

			activate(button_array[active_index]);
		}

		private function on_click(button):Function
		{
			return function(e:MouseEvent):void
			{
				activate(button);
			}
		}

		private function activate(button)
		{
			const new_color:ColorTransform = new ColorTransform();
			for (var i:int = 0; i < button_array.length; ++i)
			{
				const button_current = button_array[i];
				if (button_current != button)
				{
					new_color.color = 0x999999;
					button_current.transform.colorTransform = new_color;
				}
				else
				{
					active_index = i;
					active_name = getQualifiedClassName(button);
					Globals.active_name = active_name;
				}
			}
			new_color.color = 0x666666;
			button.transform.colorTransform = new_color;
		}
	}
}