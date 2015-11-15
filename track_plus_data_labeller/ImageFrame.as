package
{
	import flash.display.*;
	import flash.events.*;
	import flash.geom.*;
	import flash.filesystem.*;
	
	public class ImageFrame extends MovieClip
	{
		private var self:MovieClip;
		private var graphics_layer:GraphicsLayer = new GraphicsLayer();
		private var graphics_layer_colorful:GraphicsLayer = new GraphicsLayer();

		private const colors:Array = new Array();
		private const circle_buttons:Array = new Array();
		private const dot_buttons:Array = new Array();
		private const segments:Array = new Array();

		private const segments_num:int = 5;
		private var index_selected:int = 0;
		private var i:int;

		public var path:String;

		private var data_loaded:Boolean = false;
		private var initialized:Boolean = false;

		public function ImageFrame(_path:String):void
		{
			self = this;
			path = _path;

			self.addChild(graphics_layer);
			self.addChild(graphics_layer_colorful);

			for (i = 0; i < segments_num; ++i)
				segments[i] = new Point(-1, -1);

			colors[0] = 0xff0000;
			colors[1] = 0x009900;
			colors[2] = 0x0000ff;
			colors[3] = 0x990066;
			colors[4] = 0x666666;
		}

		private function init():void
		{
			if (initialized)
				return;

			initialized = true;

			function circle_button_mouse_event_handler(_circle_button:CircleButton):Function
			{
				return function(e:Event):void
				{
					index_selected = _circle_button.index;
					segments[index_selected] = new Point(-1, -1);
				};
			}

			for (i = 0; i < segments_num; ++i)
			{
				circle_buttons[i] = self["circle_button" + i];
				GraphicsUtils.tint(circle_buttons[i], colors[i], 1);
				circle_buttons[i].index = i;
				circle_buttons[i].addEventListener(MouseEvent.CLICK, circle_button_mouse_event_handler(circle_buttons[i]));
			}

			self.addEventListener(Event.ENTER_FRAME, function(e:Event):void
			{
				for (i = 0; i < segments_num; ++i)
				{
					const circle_button:CircleButton = self["circle_button" + i];

					const alpha_target:Number = i == index_selected ? 1 : 0.2;
					const alpha_diff:Number = alpha_target - circle_button.alpha;
					
					circle_button.alpha += alpha_diff / 5;
				}
			});
		}

		public function load_data():void
		{
			if (data_loaded)
				return;

			data_loaded = true;
			init();

			const file_name:String = (new File(path)).name;
			const label_file_path:String = Globals.labels_path + "/" + file_name;
			const label_exists:Boolean = FileSystem.file_exists(label_file_path);

			const data:Array = FileSystem.read_text_file(path);

			var x_old:int;
			var y_old:int;
			var x:int;
			var y:int;

			x_old = -1;
			y_old = -1;
			for (i = 0; i < data.length; ++i)
			{
				const line:String = data[i];
				const items:Array = line.split("!");
				x = parseInt(items[0]);
				y = parseInt(items[1]);
				
				if (x_old != -1)
				{
					graphics_layer.line(x_old, y_old, x, y, 0xffffff, 1);
				}
				x_old = x;
				y_old = y;

				const dot_button:DotButton = new DotButton();
				dot_button.x = x;
				dot_button.y = y;
				dot_button.index = i;
				dot_buttons[i] = dot_button;
				self.addChild(dot_button);
				
				dot_button.addEventListener(MouseEvent.CLICK, dot_button_mouse_event_handler(dot_button));
			}

			function dot_button_mouse_event_handler(_dot_button:DotButton):Function
			{
				return function(e:MouseEvent):void
				{
					if (index_selected >= segments_num)
						return;

					if (segments[index_selected].x == -1)
						segments[index_selected].x = _dot_button.index;
					else if (segments[index_selected].y == -1)
					{
						segments[index_selected].y = _dot_button.index;

						const index_min:int = Math.min(segments[index_selected].x, segments[index_selected].y);
						const index_max:int = Math.max(segments[index_selected].x, segments[index_selected].y);

						var x_old = -1;
						var y_old = -1;
						for (i = index_min; i <= index_max; ++i)
						{
							GraphicsUtils.tint(dot_buttons[i], colors[index_selected], 1);

							const x:int = dot_buttons[i].x;
							const y:int = dot_buttons[i].y;
							if (x_old != -1)
								graphics_layer_colorful.line(x_old, y_old, x, y, colors[index_selected], 1);

							x_old = x;
							y_old = y;
						}

						var finished:Boolean = false;

						++index_selected;
						if (index_selected < segments_num && !(segments[index_selected].x == -1 && segments[index_selected].y == -1))
						{
							index_selected = segments_num;
							finished = true;
						}
						else if (index_selected >= segments_num)
							finished = true;

						if (finished)
						{
							var str:String = "";
							for (i = 0; i < segments_num; ++i)
							{
								if (i > 0)
									str += "\r\n";

								str += segments[i].x + "!" + segments[i].y;
							}
							FileSystem.write_string_to_file(label_file_path, str);
						}
					}
				}
			}
			if (label_exists)
			{
				const lines:Array = FileSystem.read_text_file(label_file_path);

				var index_max_old:int = 0;
				for (i = 0; i < lines.length; ++i)
				{
					const str_line:String = lines[i];
					const str_items:Array = str_line.split("!");
					const index_min:int = parseInt(str_items[0]);
					const index_max:int = parseInt(str_items[1]);

					if (index_min != index_max_old)
					{
						graphics_layer_colorful.clear();
						break;
					}

					index_max_old = index_max;

					x_old = -1;
					y_old = -1;
					for (var a:int = index_min; a <= index_max; ++a)
					{
						GraphicsUtils.tint(dot_buttons[a], colors[index_selected], 1);

						x = dot_buttons[a].x;
						y = dot_buttons[a].y;
						if (x_old != -1)
							graphics_layer_colorful.line(x_old, y_old, x, y, colors[index_selected], 1);

						x_old = x;
						y_old = y;
					}

					++index_selected;
					if (index_selected < segments_num && !(segments[index_selected].x == -1 && segments[index_selected].y == -1))
						index_selected = segments_num;
				}
				if (index_max_old != data.length - 1)
					graphics_layer_colorful.clear();
			}

			self.background.addEventListener(MouseEvent.CLICK, function(e:MouseEvent):void
			{
				trace(file_name);
			});
		}
	}
}
