/*
 * Touch+ Software
 * Copyright (C) 2015
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the Aladdin Free Public License as
 * published by the Aladdin Enterprises, either version 9 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * Aladdin Free Public License for more details.
 *
 * You should have received a copy of the Aladdin Free Public License
 * along with this program.  If not, see <http://ghostscript.com/doc/8.54/Public.htm>.
 */

#include <opencv2/opencv.hpp>
#include <thread>
#include "globals.h"
#include "Camera.h"
#include "imu.h"
#include "mat_functions.h"
#include "camera_initializer_new.h"
#include "motion_processor_new.h"
#include "foreground_extractor_new.h"
#include "ipc.h"
#include "hand_splitter_new.h"
#include "mono_processor_new.h"
#include "pose_estimator.h"
#include "pose_estimator_new.h"
#include "reprojector.h"
#include "hand_resolver.h"
#include "pointer_mapper.h"
#include "tool_mono_processor.h"
#include "tool_stereo_processor.h"
#include "tool_pointer_mapper.h"

#include <VersionHelpers.h>

using namespace std;
using namespace cv;

//----------------------------------------structs------------------------------------------

struct Settings
{
	string launch_on_startup;
	string power_saving_mode;
	string check_for_updates;
	string touch_control;
	string table_mode;
	string auto_detect_interaction_plane;
	string scroll_bar_adjust_click_height_step;
};

//----------------------------------------instances----------------------------------------

Settings settings;

Mat image_current_frame;

IPC* ipc = NULL;

Camera* camera = NULL;

IMU imu;

MotionProcessorNew motion_processor0;
MotionProcessorNew motion_processor1;

ForegroundExtractorNew foreground_extractor0;
ForegroundExtractorNew foreground_extractor1;

HandSplitterNew hand_splitter0;
HandSplitterNew hand_splitter1;

MonoProcessorNew mono_processor0;
MonoProcessorNew mono_processor1;

PoseEstimator pose_estimator;

HandResolver hand_resolver;

Reprojector reprojector;

PointerMapper pointer_mapper;

LowPassFilter low_pass_filter;

ValueStore value_store;

ToolMonoProcessor tool_mono_processor0;
ToolMonoProcessor tool_mono_processor1;

ToolStereoProcessor tool_stereo_processor;

ToolPointerMapper tool_pointer_mapper;

const int points_pool_count_max = 1000;
vector<Point> points_pool[points_pool_count_max];
int points_pool_count = 0;
vector<Point>* points_ptr = NULL;

bool updated = false;
bool exposure_adjusted = false;
bool initialized = false;
bool calibrating = true;
bool calibration_step_2 = false;
bool first_frame = true;
bool increment_wait_count = false;
bool show_point_sent = false;
bool show_calibration_sent = false;
bool show_cursor_index = false;
bool show_cursor_thumb = false;
bool wait_for_device_bool = false;

int calibration_key_codes[4] { 56, 187, 161, 77 };
int calibration_step = 4;
int keypress_count = 0;
int increment_keypress_count_new = 0;
int increment_keypress_count_old = 0;
int wait_count = 0;
int accel_val_old = 0;
int val_diff_counter = 0;
int val_diff_old = 9999;
int frame_num = 0;
//

void wait_for_device()
{
	ipc->send_message("menu_plus", "show notification", "Device not found:Please reconnect your Touch+ module");
	ipc->send_message("menu_plus", "show stage", "true");

	while (true)
	{
		ipc->update();

		if (enable_imshow)
		{
			enable_imshow = false;
			destroyAllWindows();
		}
		
		CCameraDS camera_ds;
		int camera_count_new = camera_ds.CameraCount();
		static int camera_count_old = camera_count_new;

		if (camera_count_new != camera_count_old)
			exit(0);

		camera_count_old = camera_count_new;
		Sleep(500);
	}
}

void hide_cursors()
{
	if (!show_cursor_index && !show_cursor_thumb)
		return;

	show_cursor_index = false;
	show_cursor_thumb = false;

	ipc->send_udp_message("win_cursor_plus", "hide_cursor_index");
	ipc->send_udp_message("win_cursor_plus", "hide_cursor_thumb");
}

void update(Mat& image_in)
{
	image_current_frame = image_in;
	updated = true;
	wait_count = 0;
}

void init_paths()
{
	char buffer[MAX_PATH];
    GetModuleFileName(NULL, buffer, MAX_PATH);
    string::size_type pos = string(buffer).find_last_of("\\/");
    executable_path = string(buffer).substr(0, pos);
	data_path = executable_path + "\\userdata";
	settings_file_path = data_path + "\\settings.nrocinunerrad";
	menu_file_path = executable_path + "\\menu_plus\\menu_plus.exe";
	ipc_path = executable_path + "\\ipc";
	pose_database_path = executable_path + "\\database";
}

void load_settings()
{
	if (file_exists(settings_file_path))
	{
		ifstream ifs(settings_file_path, ios::binary);
		ifs.read((char*)&settings, sizeof(settings));

		actuate_dist = actuate_dist_raw + atoi(settings.scroll_bar_adjust_click_height_step.c_str()) - 5;

		COUT << "settings file loaded" << endl;
	}
}

void on_first_frame()
{
	bool serial_verfied = false;
	serial_number = camera->getSerialNumber();

	if (serial_number.size() == 10)
	{
		string char_string = "";
		int char_count = 0;
		for (char& c : serial_number)
		{
			char_string += c;

			++char_count;
			if (char_count == 4)
				break;
		}

		if (char_string == "0101")
			serial_verfied = true;
	}

	if (!serial_verfied)
	{
		hide_cursors();

		COUT << "please reconnect your Touch+ sensor" << endl;
		COUT << "restarting" << endl;

		wait_for_device();
	}

	data_path_current_module = data_path + "\\" + serial_number;

	int x_accel;
	int y_accel;
	int z_accel;
	camera->getAccelerometerValues(&x_accel, &y_accel, &z_accel);
	
	Point3d heading = imu.compute_azimuth(x_accel, y_accel, z_accel);

	// if (heading.y > 60)
		// mode = "tool";
	// else
		mode = "surface";

	ipc->send_message("menu_plus", "show notification", "Please wait:Initializing Touch+ Software");

	reprojector.load(*ipc);
	CameraInitializerNew::init(camera);
	pose_estimator.init();

	if (mode == "surface")
	{
		child_module_name = "win_cursor_plus";

		if (IsWindows8OrGreater())
			child_module_path = executable_path + "\\win_cursor_plus\\win_cursor_plus.exe";
		else
			child_module_path = executable_path + "\\win_cursor_plus_fallback\\win_cursor_plus.exe";

		ipc->open_udp_channel("win_cursor_plus");
		ipc->send_message("menu_plus", "show window", "");	
		ipc->send_message("menu_plus", "show wiggle", "");
	}
	else
	{
		ipc->open_udp_channel("unity_demo", 3333);
		ipc->send_message("menu_plus", "hide window", "");
	}

	ipc->map_function("toggle imshow", [](const string message_body)
	{
		if (enable_imshow)
		{
			enable_imshow = false;
			destroyAllWindows();
		}
		else
			enable_imshow = true;
	});

	ipc->map_function("load settings", [](const string message_body)
	{
		load_settings();
	});

	increment_wait_count = true;
}

void compute()
{
	ipc->update();

	if (wait_for_device_bool)
		wait_for_device();

	if (first_frame)
	{
		on_first_frame();
		first_frame = false;
	}

	if (!play || settings.touch_control != "1")
	{
		hide_cursors();

		if (enable_imshow)
			waitKey(1);

		return;
	}

	if (!updated)
		return;

	int x_accel;
	int y_accel;
	int z_accel;
	camera->getAccelerometerValues(&x_accel, &y_accel, &z_accel);
	imu.compute(x_accel, y_accel, z_accel);

	if ((mode == "surface" && imu.pitch > 60) || (mode == "tool" && imu.pitch < 60))
	{
		COUT << "exit 1" << endl;

		if (child_module_name != "")
			ipc->send_message(child_module_name, "exit", "");

		exit(0);
	}

	if (calibration_step_2)
	{
		static int calibration_step_2_count = 0;
		pointer_mapper.add_calibration_point(calibration_step);

		if (calibration_step_2_count == 10)
		{
			ipc->send_message("menu_plus", "show calibration next", "");

			COUT << "step " << calibration_step << " complete" << endl; 

			keypress_count = 0;
			++calibration_step;
			calibration_step_2 = false;
		}

		++calibration_step_2_count;
	}

	//----------------------------------------core algorithm----------------------------------------

	Mat image_flipped;
	if (mode == "surface")
		flip(image_current_frame, image_flipped, 0);
	else
		flip(image_current_frame, image_flipped, 0);
		// image_flipped = image_current_frame.clone();

	Mat image0 = image_flipped(Rect(0, 0, 640, 480));
	Mat image1 = image_flipped(Rect(640, 0, 640, 480));

	Mat image_small0;
	Mat image_small1;
	resize(image0, image_small0, Size(160, 120), 0, 0, INTER_LINEAR);
	resize(image1, image_small1, Size(160, 120), 0, 0, INTER_LINEAR);

	Mat image_preprocessed0;
	Mat image_preprocessed1;
	compute_channel_diff_image(image_small0, image_preprocessed0, exposure_adjusted, "image_preprocessed0");
	compute_channel_diff_image(image_small1, image_preprocessed1, exposure_adjusted, "image_preprocessed1");

	Mat image_preprocessed_smoothed0;
	Mat image_preprocessed_smoothed1;
	GaussianBlur(image_preprocessed0, image_preprocessed_smoothed0, Size(1, 19), 0, 0);
	GaussianBlur(image_preprocessed1, image_preprocessed_smoothed1, Size(1, 19), 0, 0);

	if (!CameraInitializerNew::adjust_exposure(camera, image_preprocessed0))
		return;

	exposure_adjusted = true;

	// imshow("image_small0", image_small0);
	// imshow("image_preprocessed0", image_preprocessed0);

	bool proceed0 = motion_processor0.compute(image_preprocessed_smoothed0, "0", false);
	bool proceed1 = motion_processor1.compute(image_preprocessed_smoothed1, "1", false);
	bool proceed = proceed0 && proceed1;

	if (proceed)
	{
		proceed0 = foreground_extractor0.compute(image_preprocessed0, image_preprocessed_smoothed0, motion_processor0, "0", true);
		proceed1 = foreground_extractor1.compute(image_preprocessed1, image_preprocessed_smoothed1, motion_processor1, "1", false);
		proceed = proceed0 && proceed1;
	}

	if (proceed)
	{
		proceed0 = hand_splitter0.compute(foreground_extractor0, motion_processor0, "0");
		proceed1 = hand_splitter1.compute(foreground_extractor1, motion_processor1, "1");
		proceed = proceed0 && proceed1;
	}

	if (mode == "surface")
	{
		if (proceed && mono_processor0.compute(hand_splitter0, image_preprocessed0, "0", false))
		{
			points_pool[points_pool_count] = mono_processor0.points_unwrapped_result;
			points_ptr = &(points_pool[points_pool_count]);

			++points_pool_count;
			if (points_pool_count == points_pool_count_max)
				points_pool_count = 0;

			initialized = true;

			if (!show_point_sent)
			{
				ipc->send_message("menu_plus", "show point", "");
				show_point_sent = true;
			}

			if (pose_name == "point" && mono_processor1.compute(hand_splitter1, image_preprocessed1, "1", false))
			{
				if (!show_calibration_sent)
				{
					ipc->send_message("menu_plus", "show calibration", "");
					show_calibration_sent = true;
				}

				hand_resolver.compute(mono_processor0, mono_processor1, motion_processor0, motion_processor1, image0, image1, reprojector);
				pointer_mapper.compute(hand_resolver, reprojector);

				if (pointer_mapper.calibrated)
				{
					if (enable_imshow)
					{
						enable_imshow = false;
						destroyAllWindows();
					}
					show_cursor_index = true;
				}

				if (show_cursor_index && pointer_mapper.thumb_down && pointer_mapper.index_down)
					show_cursor_thumb = true;
				else
					show_cursor_thumb = false;
			}
			else if (pose_name != "point")
			{
				show_cursor_index = false;
				show_cursor_thumb = false;
			}
		}

		if (!pinch_to_zoom)
		{
			if (show_cursor_index)
			{
				float pt_cursor_index_z = pointer_mapper.dist_cursor_index_plane;
				low_pass_filter.compute(pt_cursor_index_z, 0.5, "pt_cursor_index_z");

				ipc->send_udp_message("win_cursor_plus", to_string(pointer_mapper.pt_cursor_index.x) + "!" +
														 to_string(pointer_mapper.pt_cursor_index.y) + "!" +
														 to_string(pt_cursor_index_z) + "!" +
														 to_string(pointer_mapper.index_down) + "!index");
			}
			else
				ipc->send_udp_message("win_cursor_plus", "hide_cursor_index");

			ipc->send_udp_message("win_cursor_plus", "hide_cursor_thumb");
		}
		else
		{
			ipc->send_udp_message("win_cursor_plus", to_string(pointer_mapper.pt_pinch_to_zoom_index.x) + "!" +
													 to_string(pointer_mapper.pt_pinch_to_zoom_index.y) + "!0!1!index");

			ipc->send_udp_message("win_cursor_plus", to_string(pointer_mapper.pt_pinch_to_zoom_thumb.x) + "!" +
													 to_string(pointer_mapper.pt_pinch_to_zoom_thumb.y) + "!0!1!thumb");
		}

		if (increment_keypress_count_old != increment_keypress_count_new)
			++keypress_count;

		increment_keypress_count_old = increment_keypress_count_new;

		ipc->send_udp_message("win_cursor_plus", "update!" + to_string(frame_num));
	}
	else if (mode == "tool")
	{
		enable_imshow = true;

		Mat image_active_light0;
		Mat image_active_light1;
		compute_active_light_image(image_small0, image_preprocessed0, image_active_light0);
		compute_active_light_image(image_small1, image_preprocessed1, image_active_light1);

		proceed0 = tool_mono_processor0.compute(image_active_light0, image_preprocessed0, "0");
		proceed1 = tool_mono_processor1.compute(image_active_light1, image_preprocessed1, "1");
		proceed = proceed0 && proceed1;

		if (proceed)
		{
			proceed0 = tool_stereo_processor.compute(tool_mono_processor0, tool_mono_processor1);
			proceed1 = tool_stereo_processor.matches.size() >= 4;
			proceed = proceed0 && proceed1;

			if (proceed)
			{
				tool_pointer_mapper.compute(reprojector, tool_stereo_processor, image0, image1);

				string data = "";
				data += to_string(tool_pointer_mapper.pt0.x) + "!" +
						to_string(tool_pointer_mapper.pt0.y) + "!" +
						to_string(tool_pointer_mapper.pt0.z) + "!";
				data += to_string(tool_pointer_mapper.pt1.x) + "!" +
						to_string(tool_pointer_mapper.pt1.y) + "!" +
						to_string(tool_pointer_mapper.pt1.z) + "!";
				data += to_string(tool_pointer_mapper.pt2.x) + "!" +
						to_string(tool_pointer_mapper.pt2.y) + "!" +
						to_string(tool_pointer_mapper.pt2.z) + "!";
				data += to_string(tool_pointer_mapper.pt3.x) + "!" +
						to_string(tool_pointer_mapper.pt3.y) + "!" +
						to_string(tool_pointer_mapper.pt3.z) + "!";
				data += to_string(tool_pointer_mapper.pt_center.x) + "!" +
						to_string(tool_pointer_mapper.pt_center.y) + "!" +
						to_string(tool_pointer_mapper.pt_center.z);

				ipc->send_udp_message("unity_demo", data);
			}
		}
	}

	++frame_num;

	if (enable_imshow)
		waitKey(1);

	updated = false;
}

void pose_estimator_thread_function()
{
	while (true)
	{		
		if (initialized)
			pose_estimator.compute(*points_ptr);

		Sleep(100);
	}
}

HHOOK keyboard_hook_handle;

LRESULT CALLBACK keyboard_handler(int n_code, WPARAM w_param, LPARAM l_param)
{
	KBDLLHOOKSTRUCT* keyboard_hook_stuct = (KBDLLHOOKSTRUCT*)l_param;
	const int code = keyboard_hook_stuct->vkCode;

	if (target_pose_name != "")
	{
		if (code == 192)
			record_pose = true;
		else if (code == 49)
			record_pose = false;
	}
	else if (calibrating)
	{			
		if (calibration_step == 4)
			calibration_step = 0;

		if (code == calibration_key_codes[calibration_step] && pointer_mapper.active && pose_name == "point")
		{
			++increment_keypress_count_new;
			pointer_mapper.add_calibration_point(calibration_step);

			if ((keypress_count >= 10 && calibration_step != 2))
			{
				ipc->send_message("menu_plus", "show calibration next", "");

				COUT << "step " << calibration_step << " complete" << endl; 

				keypress_count = 0;
				++calibration_step;
			}
			else if (calibration_step == 2)
				calibration_step_2 = true;
		}

		if (calibration_step == 4)
		{
			ipc->send_message("menu_plus", "show stage", "");

			pointer_mapper.compute_calibration_points();
			calibrating = false;

			COUT << "calibration finished" << endl;
		}
	}
	else
		pose_name = "";

	return CallNextHookEx(keyboard_hook_handle, n_code, w_param, l_param);
}

void keyboard_hook_thread_function()
{
	keyboard_hook_handle = SetWindowsHookEx(WH_KEYBOARD_LL, keyboard_handler, NULL, 0);

	MSG msg;
    while (!GetMessage(&msg, NULL, NULL, NULL))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        Sleep(100);
    }

    UnhookWindowsHookEx(keyboard_hook_handle);
}

void input_thread_function()
{
	while (true)
	{
		string str;
		getline(cin, str);

		if (str == "set pose name")
		{
			COUT << "please enter name of the pose" << endl;

			getline(cin, str);
			target_pose_name = str;

			COUT << "pose name set to: " + target_pose_name << endl;
		}
		else if (str == "show pose name")
		{
			COUT << "started showing pose name" << endl;

			pose_estimator.show = true;
			getline(cin, str);

			COUT << "stopped showing pose name" << endl;

			pose_estimator.show = false;
		}
	}	
}

void guardian_thread_function()
{
	while (true)
	{
		if (wait_count >= 2)
		{
			hide_cursors();
			wait_for_device_bool = true;

			COUT << "restarting" << endl;

			break;
		}

		if (increment_wait_count)
			++wait_count;

		if (child_module_name != "" && process_running(child_module_name + ".exe") == 0)
			create_process(child_module_path, child_module_name + ".exe", true, true);

		Sleep(500);
	}
}

int main()
// int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	init_paths();

	thread keyboard_hook_thread(keyboard_hook_thread_function);
	thread guardian_thread(guardian_thread_function);
	thread input_thread(input_thread_function);
	thread pose_estimator_thread(pose_estimator_thread_function);

	ipc = new IPC("track_plus");
	ipc->map_function("exit", [](const string message_body)
	{
		exit(0);
	});

	camera = new Camera(true, 1280, 480, update);
	load_settings();

	while (true)
	{
		compute();

		if (settings.power_saving_mode != "1")
			Sleep(1);
		else
			Sleep(50);
	}

	return 0;
}