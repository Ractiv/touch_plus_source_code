package
{
	import flash.display.*;
	import flash.events.*;
	import flash.external.*;
	
	public class SettingsPage extends MovieClip
	{
		const self = this;

		const toggle_state_array:Array = new Array();

		public function SettingsPage():void
		{
			self.addEventListener(Event.ENTER_FRAME, function(e:Event):void
			{
				monitor_toggle(launch_on_startup);
				monitor_toggle(power_saving_mode);
				monitor_toggle(check_for_updates);
				monitor_toggle(touch_control);
				monitor_toggle(table_mode);
			});
		}

		private function monitor_toggle(toggle:SettingsItem):void
		{
			if (toggle_state_array[toggle.name] != undefined && toggle_state_array[toggle.name] != toggle.toggle_enabled)
			{
				ExternalInterface.call("toggle_clicked", toggle.name, toggle.toggle_enabled);
			}
			toggle_state_array[toggle.name] = toggle.toggle_enabled;
		}
	}
}
