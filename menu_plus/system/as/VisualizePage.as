package
{
	import flash.display.*;
	import flash.external.*;
	import flash.events.*;
	
	public class VisualizePage extends MovieClip
	{
		public function VisualizePage():void
		{
			button_toggle_visualizer.addEventListener(MouseEvent.CLICK, function(e:MouseEvent):void
			{
				ExternalInterface.call("toggle_imshow");
			});
		}
	}
}
