package
{
	public class Globals
	{
		private var self = this;
		
		public static var stage_width:int = 640;
		public static var stage_height:int = 675;
		public static var active_name:String;

		public static var text_bubble:TextBubble = new TextBubble();

		public static function map_val(value:Number, left_min:Number, left_max:Number, right_min:Number, right_max:Number):Number
		{
		    var left_span:Number = left_max - left_min;
		    var right_span:Number = right_max - right_min;
		    var value_scaled:Number = (value - left_min) / left_span;
		    return right_min + (value_scaled * right_span);
		}
	}
}