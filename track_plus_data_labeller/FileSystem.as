package
{
	import flash.filesystem.*;

	public class FileSystem
	{
		public function FileSystem():void
		{
			// constructor code
		}

		public static function directory_exists(path:String):Boolean
		{
			var folder:File = File.userDirectory.resolvePath(path);
			return folder.exists;
		}

		public static function file_exists(path:String):Boolean
		{
			return directory_exists(path);
		}

		public static function list_files_in_directory(path:String):Vector.<String>
		{
			const desktop:File = File.applicationDirectory.resolvePath(path);
			const files:Array = desktop.getDirectoryListing();

			const file_name_vec:Vector.<String> = new Vector.<String>();
			for (var i:int = 0; i < files.length; ++i)
				file_name_vec.push(files[i].name);

			return file_name_vec;
		}

		public static function list_files_in_directory0(path:String):Array
		{
			const desktop:File = File.applicationDirectory.resolvePath(path);
			const files:Array = desktop.getDirectoryListing();
			return files;
		}

		public static function create_directory(path:String):void
		{
			const target:File = File.documentsDirectory.resolvePath(path);
			if (!target.exists)
				target.createDirectory();
		}

		public static function delete_directory(path:String):void
		{
			const directory:File = File.documentsDirectory.resolvePath(path);
			if (directory.exists)
				directory.deleteDirectory(true);
		}

		public static function write_string_to_file(path:String, str:String):void
		{
			const file_to_create:File = File.documentsDirectory.resolvePath(path);
			const file_stream:FileStream = new FileStream();
			file_stream.open(file_to_create, FileMode.WRITE);
			file_stream.writeUTFBytes(str);
			file_stream.close();
		}

		public static function read_text_file(path:String):Array
		{
			const my_file:File = File.documentsDirectory.resolvePath(path);
			const file_stream:FileStream = new FileStream();
			file_stream.open(my_file, FileMode.READ);
			const str:String = file_stream.readUTFBytes(file_stream.bytesAvailable);
			file_stream.close();

			const lines:Array = str.split(/[\u000d\u000a\u0008]+/g);
			return lines;
		}

		public static function copy_file(src_path:String, dst_path:String):void
		{
			const old_file:File = File.documentsDirectory.resolvePath(src_path);
			const new_file:File = File.documentsDirectory.resolvePath(dst_path); 
			old_file.copyTo(new_file, true); 
		}

		public static function delete_file(path:String):void
		{
			var file:File = File.documentsDirectory.resolvePath(path);
			if (file.exists)
				file.deleteFile();
		}

		public static function delete_all_files(path:String):void
		{
			delete_directory(path);
			create_directory(path);
		}
	}
}
