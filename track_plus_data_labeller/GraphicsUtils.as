package
{
	import flash.display.*;
	import fl.motion.*;
	
	public class GraphicsUtils
	{
		public static function tint(obj:DisplayObject, color:Number, alpha:Number):void
		{
			var c:Color = new Color();
			c.setTint(color, alpha);
			obj.transform.colorTransform = c;
		}
	}
}
