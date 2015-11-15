package
{
	public class MathPlus
	{
		public static function map_val(value:Number, left_min:Number, left_max:Number, right_min:Number, right_max:Number):Number
		{
			const left_span:Number = left_max - left_min;
		    const right_span:Number = right_max - right_min;
		    const value_scaled:Number = (value - left_min) / left_span;
		    return right_min + (value_scaled * right_span);
		}
	}
}