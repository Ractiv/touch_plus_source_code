package
{
	import flash.filesystem.*;

	public class Globals
	{
		public static const current_path:String = File.applicationDirectory.nativePath;
		public static const executable_path:String = (new File(current_path)).resolvePath("./../build/").nativePath;
		public static const database_path:String = executable_path + "/database";
		public static const labels_path:String = database_path + "/labels";
	}
}
