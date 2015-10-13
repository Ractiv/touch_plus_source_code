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
#include <opencv2/calib3d/calib3d.hpp>
#include <thread>

#include "globals.h"
#include "ipc.h"
#include "Camera.h"
#include "imu.h"
#include "surface_computer.h"
#include "mat_functions.h"
#include "camera_initializer_new.h"
#include "motion_processor_new.h"
#include "foreground_extractor_new.h"
#include "hand_splitter_new.h"
#include "mono_processor_new.h"
#include "stereo_processor.h"
#include "pose_estimator.h"
#include "reprojector.h"
#include "hand_resolver.h"
#include "point_resolver.h"
#include "pointer_mapper.h"
#include "processes.h"
#include "console_log.h"

#ifdef _WIN32
#include <VersionHelpers.h>

#elif __APPLE__
#include <mach-o/dyld.h>
#endif

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
    string detect_interaction_plane;
    string click_height;
};

//----------------------------------------instances----------------------------------------

Settings settings;

Mat image_current_frame;

IPC* ipc = NULL;

Camera* camera = NULL;

IMU imu;

SurfaceComputer surface_computer;

MotionProcessorNew motion_processor0;
MotionProcessorNew motion_processor1;

ForegroundExtractorNew foreground_extractor0;
ForegroundExtractorNew foreground_extractor1;

HandSplitterNew hand_splitter0;
HandSplitterNew hand_splitter1;

MonoProcessorNew mono_processor0;
MonoProcessorNew mono_processor1;

StereoProcessor stereo_processor;

PoseEstimator pose_estimator;

Reprojector reprojector;

HandResolver hand_resolver;

PointResolver point_resolver = PointResolver(motion_processor0, motion_processor1, reprojector);

PointerMapper pointer_mapper;

LowPassFilter low_pass_filter;

const int pool_size_max = 10;

Mat image_pool[pool_size_max];
int image_pool_count = 0;

vector<Point> point_vec_pool[pool_size_max];
int point_vec_pool_count = 0;
vector<Point>* point_vec_ptr = NULL;

bool serial_verified = false;
bool initialized = false;
bool updated = false;

int wait_count = 0;
int frame_count = 0;
//

void do_exit(bool kill_child)
{
    if (child_module_name != "" && kill_child)
        ipc->send_message(child_module_name, "exit", "");

    ipc->clear();
        
#ifdef __APPLE__
    if (camera != NULL)
        camera->stopVideoStream();
#endif

    console_log("exit");
    exit(0);
}

void wait_for_device()
{
    console_log("waiting for device ");
    ipc->send_message("menu_plus", "reset progress", "");

#ifdef __APPLE__
    string command = "ioreg -p IOUSB -w0 | sed 's/[^o]*o //; s/@.*$//' | grep -v '^Root.*' > " + executable_path + "/devices.txt";
    system(command.c_str());
    
    vector<string> lines = read_text_file(executable_path + "/devices.txt");
    delete_file(executable_path + "/devices.txt");
    
    for (string& str : lines)
        if (str == "Touch+ Camera")
        {
            ipc->send_message("menu_plus", "set status", "communicating");
            ipc->send_message("menu_plus", "show notification", "Please wait:Attempting to communicate with Touch+");

            do_exit(false);
        }
#endif

    ipc->send_message("menu_plus", "show debug page", "");
    ipc->send_message("menu_plus", "set status", "error: device not found");
    ipc->send_message("menu_plus", "show notification", "Device not found:Please reconnect your Touch+ module");

    while (true)
    {
        if (enable_imshow)
            enable_imshow = false;
        
#ifdef _WIN32
        CCameraDS camera_ds;
        int camera_count_new = camera_ds.CameraCount();
        static int camera_count_old = camera_count_new;

        if (camera_count_new > camera_count_old)
        {
            ipc->send_message("menu_plus", "set status", "restarting tracking core");
            do_exit(false);
        }
        camera_count_old = camera_count_new;

#elif __APPLE__
        string command = "ioreg -p IOUSB -w0 | sed 's/[^o]*o //; s/@.*$//' | grep -v '^Root.*' > " + executable_path + "/devices.txt";
        system(command.c_str());
        
        vector<string> lines = read_text_file(executable_path + "/devices.txt");
        delete_file(executable_path + "/devices.txt");

        for (string& str : lines)
            if (str == "Touch+ Camera")
            {
                ipc->send_message("menu_plus", "set status", "restarting tracking core");
                do_exit(false);
            }
#endif

        Sleep(500);
    }
}

void update(Mat& image_in)
{
    image_pool[image_pool_count] = image_in;
	image_current_frame = image_pool[image_pool_count];
    
    ++image_pool_count;
    if (image_pool_count == pool_size_max)
        image_pool_count = 0;

    wait_count = 0;
    updated = true;
}

void init_paths()
{
#ifdef _WIN32
    char buffer[MAX_PATH];
    GetModuleFileName(NULL, buffer, MAX_PATH);
    string::size_type pos = string(buffer).find_last_of("\\/");
    executable_path = string(buffer).substr(0, pos);

#elif __APPLE__
    char path_buffer[1024];
    uint32_t path_size = sizeof(path_buffer);
    _NSGetExecutablePath(path_buffer, &path_size);
    string path_str(path_buffer);
    executable_path = path_str.substr(0, path_str.find_last_of("/"));
#endif

    data_path = executable_path + slash + "userdata";
    settings_file_path = data_path + slash + "settings.nrocinunerrad";
    ipc_path = executable_path + slash + "ipc";
    pose_database_path = executable_path + slash + "database";
}

void load_settings()
{
    if (file_exists(settings_file_path))
    {
        ifstream ifs(settings_file_path, ios::binary);
        ifs.read((char*)&settings, sizeof(settings));
        actuate_dist = actuate_dist_raw + atoi(settings.click_height.c_str()) - 5;

        console_log("settings file loaded");
    }
}

void setup_on_first_frame()
{
    console_log("on first frame");

    ipc->send_message("menu_plus", "set status", "verifying serial number");
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
            serial_verified = true;
    }

    if (!serial_verified)
        wait_for_device();

    ipc->send_message("menu_plus", "set loading progress", "10");
    console_log("serial number: " + serial_number);

    data_path_current_module = data_path + slash + serial_number;

    int x_accel;
    int y_accel;
    int z_accel;
    camera->getAccelerometerValues(&x_accel, &y_accel, &z_accel);    
    
    Point3d heading = imu.compute_azimuth(x_accel, y_accel, z_accel);

    ipc->send_message("menu_plus", "set status", "loading calibration data");
    reprojector.load(*ipc, true);
    ipc->send_message("menu_plus", "set loading progress", "40");

    ipc->send_message("menu_plus", "set status", "initializing camera");
    CameraInitializerNew::init(camera);

    ipc->send_message("menu_plus", "set status", "loading pose data");
    pose_estimator.init();
    ipc->send_message("menu_plus", "set loading progress", "70");

    ipc->send_message("menu_plus", "set status", "starting cursor subprocess");

#ifdef _WIN32
    child_module_name = "win_cursor_plus";
    
    if (IsWindows8OrGreater())
        child_module_path = executable_path + slash + "win_cursor_plus" + slash + "win_cursor_plus" + extension0;
    else
        child_module_path = executable_path + slash + "win_cursor_plus_fallback" + slash + "win_cursor_plus" + extension0;

#elif __APPLE__
    //todo: port to OSX
#endif

    ipc->map_function("toggle imshow", [](const string message_body)
    {
        if (enable_imshow)
            enable_imshow = false;
        else
            enable_imshow = true;
    });

    ipc->map_function("load settings", [](const string message_body)
    {
        load_settings();
    });

    ipc->send_message("menu_plus", "set loading progress", "100");
}

void left_mouse_cb(int event, int x, int y, int flags, void* userdata)
{
    if (event == EVENT_LBUTTONDOWN)
        console_log(to_string(x) + ", " + to_string(y) + " " + to_string(imu.pitch));
}

void compute()
{
    ++frame_count;
    updated = false;

    if (image_current_frame.cols == 0)
    {
        console_log("bad frame");
        return;
    }

    Mat image_flipped;
    flip(image_current_frame, image_flipped, 0);

    Mat image0 = image_flipped(Rect(0, 0, 640, 480));
    Mat image1 = image_flipped(Rect(640, 0, 640, 480));

    if (frame_count == 1)
        setup_on_first_frame();

    if (!play || settings.touch_control != "1")
    {
        if (enable_imshow)
            waitKey(1);

        return;
    }

    int x_accel;
    int y_accel;
    int z_accel;
    camera->getAccelerometerValues(&x_accel, &y_accel, &z_accel);
    imu.compute(x_accel, y_accel, z_accel);

    //----------------------------------------core algorithm----------------------------------------

    Mat image_small0;
    Mat image_small1;
    resize(image0, image_small0, Size(160, 120), 0, 0, INTER_LINEAR);
    resize(image1, image_small1, Size(160, 120), 0, 0, INTER_LINEAR);

    Mat image_preprocessed0;
    Mat image_preprocessed1;

    static bool exposure_set = false;

    bool normalized = compute_channel_diff_image(image_small0, image_preprocessed0, exposure_set, "image_preprocessed0", true, exposure_set);
                      compute_channel_diff_image(image_small1, image_preprocessed1, exposure_set, "image_preprocessed1");

    GaussianBlur(image_preprocessed0, image_preprocessed0, Size(3, 9), 0, 0);
    GaussianBlur(image_preprocessed1, image_preprocessed1, Size(3, 9), 0, 0);

    if (!CameraInitializerNew::adjust_exposure(camera, image_preprocessed0))
    {
        static int step_count = 0;
        if (step_count == 3)
            surface_computer.init(image0);

        ++step_count;
        return;
    }

    exposure_set = true;

#if 0
    {
        Mat image_remapped0 = reprojector.remap(&image_small0, 0, true);
        Mat image_remapped1 = reprojector.remap(&image_small1, 1, true);

        reprojector.y_align(image_remapped0, image_remapped1, false);

        GaussianBlur(image_remapped0, image_remapped0, Size(9, 9), 0, 0);
        GaussianBlur(image_remapped1, image_remapped1, Size(9, 9), 0, 0);


		Mat image_disparity;

		static StereoSGBM stereo_sgbm(0, 32, 21, 0, 0, 0, 0, 20, 0, 0, false);
		stereo_sgbm(image_remapped0, image_remapped1, image_disparity);

        Mat image_disparity_8u;
        double minVal, maxVal;
        minMaxLoc(image_disparity, &minVal, &maxVal);
        image_disparity.convertTo(image_disparity_8u, CV_8UC1, 255 / (maxVal - minVal));

		imshow("image_remapped0", image_remapped0);
		imshow("image_remapped1", image_remapped1);
		imshow("image_disparity_8u", image_disparity_8u);
    }
#endif

	/*static bool show_wiggle_sent = false;
	if (!show_wiggle_sent)
	{
        if (child_module_name != "")
		  ipc->open_udp_channel(child_module_name);
        
		ipc->send_message("menu_plus", "show window", "");
		ipc->send_message("menu_plus", "show wiggle", "");//todo
	}
	show_wiggle_sent = true;*/

    if (enable_imshow)
    {
        imshow("image_small1", image_small1);
        imshow("image_preprocessed1", image_preprocessed1);

        // setMouseCallback("image_preprocessed1", left_mouse_cb, NULL);
        // waitKey(1);
        // return;
    }

    algo_name_vec_old = algo_name_vec;
	algo_name_vec.clear();

    static bool motion_processor_proceed = false;
    static bool construct_background = false;
    static bool first_pass = true;

    bool proceed0;
    bool proceed1;

    if (normalized)
    {
        proceed0 = motion_processor0.compute(image_preprocessed0,  image_small0, surface_computer.y_reflection, imu.pitch,
                                             construct_background, "0",          false);
        proceed1 = motion_processor1.compute(image_preprocessed1,  image_small1, surface_computer.y_reflection, imu.pitch,
                                             construct_background, "1",          false);
    }

    if (first_pass && motion_processor0.both_moving && motion_processor1.both_moving)
    {
        console_log("readjusting exposure");

        first_pass = false;
        exposure_set = false;
        mat_functions_low_pass_filter.reset();

        CameraInitializerNew::adjust_exposure(camera, image_preprocessed0, true);
    }
    else if (!first_pass)
        construct_background = true;

    if (!construct_background)
    {
        proceed0 = false;
        proceed1 = false;            
    }

    if (proceed0 && proceed1)
        motion_processor_proceed = true;

    bool proceed = motion_processor_proceed;

    static bool menu_plus_signal0 = false;
    if (!menu_plus_signal0)
    {
        menu_plus_signal0 = true;
        // ipc->send_message("menu_plus", "hide window", "");
    }

    if (proceed)
    {
        proceed0 = foreground_extractor0.compute(image_preprocessed0, motion_processor0, "0", false);
        proceed1 = foreground_extractor1.compute(image_preprocessed1, motion_processor1, "1", false);
        proceed = proceed0 && proceed1;
    }

    if (proceed)
    {
        proceed0 = hand_splitter0.compute(foreground_extractor0, motion_processor0, "0", false);
        proceed1 = hand_splitter1.compute(foreground_extractor1, motion_processor1, "1", false);
        proceed = proceed0 && proceed1;
    }

    if (proceed)
    {
        proceed0 = mono_processor0.compute(hand_splitter0, "0", true);
        proceed1 = mono_processor1.compute(hand_splitter1, "1", false);
        proceed = proceed0 && proceed1;
    }

    if (proceed)
        stereo_processor.compute(mono_processor0, mono_processor1, point_resolver, pointer_mapper, image0, image1);

    if (enable_imshow)
        waitKey(1);
}

void pose_estimator_thread_function()
{
    while (true)
    {
        if (initialized)
            pose_estimator.compute(*point_vec_ptr);

        Sleep(100);
    }
}

void input_thread_function()
{
    while (true)
    {
        string str;
        getline(cin, str);

        if (str == "set pose name")
        {
            console_log("please enter name of the pose");

            getline(cin, str);
            target_pose_name = str;

            console_log("pose name set to: " + target_pose_name);
        }
        else if (str == "show pose name")
        {
            console_log("started showing pose name");

            pose_estimator.show = true;
            getline(cin, str);

            console_log("stopped showing pose name");

            pose_estimator.show = false;
        }
        else if (str == "set exposure")
        {
            console_log("please enter exposure value");

            getline(cin, str);
            camera->setExposureTime(Camera::both, atoi(str.c_str()));

            console_log("exposure value set to: " + str);
        }
    }
}

void guardian_thread_function()
{
    while (true)
    {
        if (wait_count >= (serial_verified ? 5 : 10))
            wait_for_device();

        ++wait_count;

        /*if (child_module_name != "" && child_module_path != "")
        {
            static bool first = true;
            if (first && (process_running(child_module_name + extension0) > 0))
            {
                kill_process(child_module_name + extension0);
                while (process_running(child_module_name + extension0) > 0)
                {
                    console_log("wait kill");
                    Sleep(100);
                }
            }
            first = false;
        }*/

        // if (child_module_name != "" && child_module_path != "" && process_running(child_module_name + extension0) == 0)
            // create_process(child_module_path, child_module_name + extension0, true);

        if (process_running("daemon_plus" + extension0) == 0)
        {
            ipc->send_message("everyone", "exit", "");
            do_exit(true);
        }

        Sleep(500);
    }
}

void ipc_thread_function()
{
    while (true)
    {
        ipc->update();
        Sleep(100);
    }
}

int main()
{
    init_paths();

    ipc = new IPC("track_plus");
    console_log_ipc = ipc;
    thread ipc_thread(ipc_thread_function);

    ipc->map_function("exit", [](const string message_body)
    {
        do_exit(true);
    });

    bool menu_plus_ready = false;
    bool* menu_plus_ready_ptr = &menu_plus_ready;
    ipc->map_function("menu_plus_ready", [menu_plus_ready_ptr](const string message_body)
    {
        *menu_plus_ready_ptr = true;
    });

    while (!menu_plus_ready)
    {
        ipc->send_message("menu_plus", "menu_plus_ready", "");
        Sleep(1000);
    }

    ipc->send_message("menu_plus", "show debug page", "");
    ipc->send_message("menu_plus", "show notification", "Please wait:Initializing Touch+ Software");
    ipc->send_message("menu_plus", "set status", "initializing tracking core");
    ipc->open_udp_channel("menu_plus");

    thread guardian_thread(guardian_thread_function);
    thread input_thread(input_thread_function);
    thread pose_estimator_thread(pose_estimator_thread_function);

    camera = new Camera(true, 1280, 480, update);
    
    load_settings();

    while (true)
    {
        static bool enable_imshow_old = enable_imshow;
        if (enable_imshow_old && !enable_imshow)
            destroyAllWindows();

        enable_imshow_old = enable_imshow;

        if (!updated)
        {
            Sleep(1);
            continue;
        }

        compute();

        if (settings.power_saving_mode != "1")
            Sleep(1);
        else
            Sleep(50);
    }
        
    return 0;
}