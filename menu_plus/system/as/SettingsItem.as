package
{
	import flash.display.*;
	import flash.geom.*
	
	public class SettingsItem extends MovieClip
	{
		private const self = this;
		private var is_enabled:Boolean = true;
		private var current_type:String = "toggle";

		[Inspectable(name = "label0", type = String)]
		public function set label0(val:String):void
		{
			line0.text = val;
		}
		public function get label0():String
		{
           return line0.text;
      	}

      	[Inspectable(name = "label1", type = String)]
		public function set label1(val:String):void
		{
			line1.text = val;
		}
		public function get label1():String
		{
           return line1.text;
      	}

      	[Inspectable(name = "active", type = Boolean)]
		public function set active(val:Boolean):void
		{
			var new_color:ColorTransform = new ColorTransform();
			if (val)
			{
				// new_color.color = 0x666666;
				// self.transform.colorTransform = new_color;
				// self.mouseEnabled = true;
				// self.mouseChildren = true;
			}
			else
			{
				new_color.color = 0x999999;
				self.transform.colorTransform = new_color;
				self.mouseEnabled = false;
				self.mouseChildren = false;
			}
		}
		public function get active():Boolean
		{
			return is_enabled;
      	}
		
		[Inspectable(name = "item_type", type = List)]
		public function set item_type(val:String):void
		{
			if (val == "toggle")
			{
				toggle.visible = true;
				scroll_bar.visible = false;
			}
			else if (val == "scroll_bar")
			{
				toggle.visible = false;
				scroll_bar.visible = true;
			}
		}
		public function get item_type():String
		{
			return current_type;
      	}

      	[Inspectable(name = "toggle_enabled", type = Boolean)]
		public function set toggle_enabled(val:Boolean):void
		{
			toggle.is_on = val;
		}
		public function get toggle_enabled():Boolean
		{
			return toggle.is_on;
      	}

      	[Inspectable(name = "scroll_bar_level", type = Number)]
		public function set scroll_bar_level(val:Number):void
		{
			scroll_bar.set_level(val);
		}
		public function get scroll_bar_level():Number
		{
			return scroll_bar.level_current;
      	}

		public function SettingsItem():void
		{
		}
	}
}
