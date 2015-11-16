package
{
	import flash.display.*;
	import flash.events.*
	import flash.geom.*
	
	public class Main extends MovieClip
	{
		private var self:MovieClip;
		private var mouse_down:Boolean = false;
		private var mouse_y_offset:Number = 9999;

		public function Main():void
		{
			self = this;
			var i:int;

			const container:Sprite = new Sprite();
			self.addChild(container);

			const margin:int = 7;
			scroller.y = margin;

			scroller.addEventListener(MouseEvent.MOUSE_DOWN, function(e:MouseEvent):void
			{
				mouse_down = true;
			});

			scroller.addEventListener(MouseEvent.MOUSE_UP, function(e:MouseEvent):void
			{
				mouse_down = false;
			});

			scroller.addEventListener(MouseEvent.RELEASE_OUTSIDE, function(e:MouseEvent):void
			{
				mouse_down = false;
			});

			if (!FileSystem.directory_exists(Globals.labels_path))
				FileSystem.create_directory(Globals.labels_path);

			const files_vec:Array = FileSystem.list_files_in_directory0(Globals.database_path);

			const image_frame_width:int = 165;
			const image_frame_height:int = 145;

			const image_frames:Vector.<ImageFrame> = new Vector.<ImageFrame>();

			var x_last:int = margin;
			var y_last:int = margin;
			for (i = 0; i < files_vec.length; ++i)
			{
				if (files_vec[i].extension != "nrocinunerrad")
					continue;

				const image_frame:ImageFrame = new ImageFrame(files_vec[i].nativePath);
				image_frame.x = x_last;
				image_frame.y = y_last;
				container.addChild(image_frame);

				x_last += image_frame_width;
				if (x_last > stage.stageWidth - image_frame_width)
				{
					x_last = margin;
					y_last += image_frame_height;
				}

				image_frames.push(image_frame);
			}

			const container_y_min:int = -container.height + stage.stageHeight - margin - margin;
			const top:int = margin;
			const bottom:int = stage.stageHeight - margin;
			const scroller_y_min = top;
			const scroller_y_max = bottom - scroller.height;

			self.addEventListener(Event.ENTER_FRAME, function(e:Event):void
			{
				if (mouse_down)
				{
					if (mouse_y_offset == 9999)
						mouse_y_offset = scroller.y - stage.mouseY;

					scroller.y = stage.mouseY + mouse_y_offset;
					if (scroller.y < top)
						scroller.y = top;
					if (scroller.y + scroller.height > bottom)
						scroller.y = bottom - scroller.height;

					const container_y:Number = MathPlus.map_val(scroller.y, scroller_y_min, scroller_y_max, top, container_y_min);
					container.y = container_y;
				}
				else
				{
					mouse_y_offset = 9999;
				}

				for (i = 0; i < image_frames.length; ++i)
				{
					const image_frame:ImageFrame = image_frames[i];
					const global_pos:Point = container.localToGlobal(new Point(image_frame.x, image_frame.y));
					if (global_pos.y < stage.stageHeight && global_pos.y + image_frame_height > 0)
					{
						image_frame.visible = true;

						// if (!mouse_down)
							image_frame.load_data();
					}
					else
					{
						image_frame.visible = false;
					}
				}
			});
		}
	}
}
