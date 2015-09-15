package
{
	import flash.external.*;

	public class Interop
	{
		private static var cb_count:int = 0;

		public static function call_js(js_code, cb):void
		{
			var cb_name:String = "cb" + cb_count.toString();
			ExternalInterface.addCallback(cb_name, cb);
			++cb_count;
			ExternalInterface.call("gui." + cb_name + "(" + js_code + ")");
		}

		public static function log(content):void
		{
			ExternalInterface.call("console.log('" + content + "')");
		}
	}
}