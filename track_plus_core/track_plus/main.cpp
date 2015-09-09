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
#include "pointer_mapper.h"

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
    string auto_detect_interaction_plane;
    string scroll_bar_adjust_click_height_step;
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

HandResolver hand_resolver;

Reprojector reprojector;

PointerMapper pointer_mapper;

LowPassFilter low_pass_filter;

const int points_pool_count_max = 1000;
vector<Point> points_pool[points_pool_count_max];
int points_pool_count = 0;
vector<Point>* points_ptr = NULL;

bool serial_verified = false;
bool exposure_adjusted = false;
bool initialized = false;
bool calibrating = true;
bool increment_keypress_count = false;
bool first_frame = true;
bool show_point_sent = false;
bool show_calibration_sent = false;
bool show_cursor_index = false;
bool show_cursor_thumb = false;
bool updated = false;

int calibration_key_codes[4] { 56, 187, 161, 77 };
int calibration_step = 0;
int calibration_points_count = 20;
int keypress_count = 0;
int wait_count = 0;
int accel_val_old = 0;
int val_diff_counter = 0;
int val_diff_old = 9999;
int frame_num = 0;
//

void hide_cursors()
{
    if ((!show_cursor_index && !show_cursor_thumb) || child_module_name == "")
        return;

    show_cursor_index = false;
    show_cursor_thumb = false;

    ipc->send_udp_message(child_module_name, "hide_cursor_index");
    ipc->send_udp_message(child_module_name, "hide_cursor_thumb");
}

void wait_for_device()
{
    COUT << "waiting for device " << endl;

    hide_cursors();
    
#ifdef __APPLE__
    string command = "ioreg -p IOUSB -w0 | sed 's/[^o]*o //; s/@.*$//' | grep -v '^Root.*' > " + executable_path + "/devices.txt";
    system(command.c_str());
    
    vector<string> lines = read_text_file(executable_path + "/devices.txt");
    delete_file(executable_path + "/devices.txt");
    
    for (string& str : lines)
        if (str == "Touch+ Camera")
        {
            ipc->send_message("menu_plus", "show notification", "Please wait:Attempting to communicate with Touch+");
            ipc->send_message("menu_plus", "show stage", "");
            
            ipc->clear();
            if (camera != NULL)
                camera->stopVideoStream();
            
            COUT << "exit 0" << endl;
            exit(0);
        }
#endif

    ipc->send_message("menu_plus", "show notification", "Device not found:Please reconnect your Touch+ module");
    ipc->send_message("menu_plus", "show stage", "");

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
            ipc->clear();
#ifdef __APPLE__
            if (camera != NULL)
                camera->stopVideoStream();
#endif
            COUT << "exit 0" << endl;

            exit(0);
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
                ipc->clear();
                if (camera != NULL)
                    camera->stopVideoStream();
                
                COUT << "exit 0" << endl;
                exit(0);
            }
#endif

        Sleep(500);
    }
}

void update(Mat& image_in)
{
    image_current_frame = image_in;
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
        actuate_dist = actuate_dist_raw + atoi(settings.scroll_bar_adjust_click_height_step.c_str()) - 5;

        COUT << "settings file loaded" << endl;
    }
}

void on_first_frame()
{
    COUT << "on first frame" << endl;

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
    {
        COUT << "please reconnect your Touch+ sensor" << endl;
        wait_for_device();
    }

    COUT << "serial number: " << serial_number << endl;

    data_path_current_module = data_path + slash + serial_number;

    int x_accel;
    int y_accel;
    int z_accel;
    camera->getAccelerometerValues(&x_accel, &y_accel, &z_accel);    
    
    Point3d heading = imu.compute_azimuth(x_accel, y_accel, z_accel);

    reprojector.load(*ipc, true);
    CameraInitializerNew::init(camera);
    pose_estimator.init();

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
}

void compute()
{
    updated = false;

    Mat image_flipped;
    flip(image_current_frame, image_flipped, 0);

    Mat image0 = image_flipped(Rect(0, 0, 640, 480));
    Mat image1 = image_flipped(Rect(640, 0, 640, 480));

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

    int x_accel;
    int y_accel;
    int z_accel;
    camera->getAccelerometerValues(&x_accel, &y_accel, &z_accel);
    imu.compute(x_accel, y_accel, z_accel);

    if (image_current_frame.cols == 0)
        return;

    //----------------------------------------core algorithm----------------------------------------

    Mat image_small0;
    Mat image_small1;
    resize(image0, image_small0, Size(160, 120), 0, 0, INTER_LINEAR);
    resize(image1, image_small1, Size(160, 120), 0, 0, INTER_LINEAR);

    Mat image_preprocessed0;
    Mat image_preprocessed1;

    bool normalized = compute_channel_diff_image(image_small0, image_preprocessed0, exposure_adjusted, "image_preprocessed0", true);
                      compute_channel_diff_image(image_small1, image_preprocessed1, exposure_adjusted, "image_preprocessed1", false);

    GaussianBlur(image_preprocessed0, image_preprocessed0, Size(3, 9), 0, 0);
    GaussianBlur(image_preprocessed1, image_preprocessed1, Size(3, 9), 0, 0);

    if (!CameraInitializerNew::adjust_exposure(camera, image_small0))
    {
        static int step_count = 0;
        if (step_count == 3)
        {
            surface_computer.init(image0);
        }
        ++step_count;
        return;
    }

    exposure_adjusted = true;

#ifdef false
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

	static bool show_wiggle_sent = false;
	if (!show_wiggle_sent)
	{
        if (child_module_name != "")
		  ipc->open_udp_channel(child_module_name);
        
		ipc->send_message("menu_plus", "show window", "");
		ipc->send_message("menu_plus", "show wiggle", "");
	}
	show_wiggle_sent = true;

    if (enable_imshow)
    {
        imshow("image_small1", image_small1);
        imshow("image_preprocessed1", image_preprocessed1);
    }

    static bool motion_processor_proceed = false;
    static bool construct_background = false;
    static bool first_pass = true;

    bool proceed0;
    bool proceed1;

    if (normalized)
    {
        proceed0 = motion_processor0.compute(image_preprocessed0,  image_small0, surface_computer.y_reflection, imu.pitch,
                                             construct_background, "0",          true);
        proceed1 = motion_processor1.compute(image_preprocessed1,  image_small1, surface_computer.y_reflection, imu.pitch,
                                             construct_background, "1",          true);
    }

    if (first_pass && motion_processor0.both_moving)
    {
        COUT << "readjusting exposure" << endl;

        first_pass = false;
        exposure_adjusted = false;
        CameraInitializerNew::adjust_exposure(camera, image_small0, true);
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

    if (proceed)
    {
        proceed0 = foreground_extractor0.compute(image_preprocessed0, motion_processor0, "0", false);
        proceed1 = foreground_extractor1.compute(image_preprocessed1, motion_processor1, "1", false);
        proceed = proceed0 && proceed1;
    }

    if (proceed)
    {
        proceed0 = hand_splitter0.compute(foreground_extractor0, motion_processor0, "0");
        proceed1 = hand_splitter1.compute(foreground_extractor1, motion_processor1, "1");
        proceed = proceed0 && proceed1;
    }

    if (proceed)
    {
        proceed0 = mono_processor0.compute(hand_splitter0, "0", true);
        proceed1 = mono_processor1.compute(hand_splitter1, "1", false);
        proceed = proceed0 && proceed1;

        if (proceed)
        {
            stereo_processor.compute(mono_processor0, mono_processor1, reprojector, pointer_mapper);

            points_pool[points_pool_count] = mono_processor0.pose_estimation_points;
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

            if (pose_name == "point")
            {
                if (!show_calibration_sent)
                {
                    ipc->send_message("menu_plus", "show calibration", "");
                    show_calibration_sent = true;
                }

                hand_resolver.compute(mono_processor0,   mono_processor1,
                                      motion_processor0, motion_processor1,
                                      image0,            image1,
                                      reprojector,       false);

                pointer_mapper.compute(hand_resolver, reprojector);

                if (pointer_mapper.calibrated)
                    show_cursor_index = true;

                if (show_cursor_index && pointer_mapper.thumb_down && pointer_mapper.index_down)
                    show_cursor_thumb = true;
                else
                    show_cursor_thumb = false;
            }
            else
            {
                pointer_mapper.reset();

                if (pose_name != "point")
                {
                    show_cursor_index = false;
                    show_cursor_thumb = false;
                }
            }
        }

        if (!pinch_to_zoom)
        {
            if (show_cursor_index)
            {
                ipc->send_udp_message(child_module_name, to_string(pointer_mapper.pt_cursor_index.x) + "!" +
                                                         to_string(pointer_mapper.pt_cursor_index.y) + "!" +
                                                         // "20!" +
                                                         to_string(pointer_mapper.dist_cursor_index_plane) + "!" +
                                                         to_string(pointer_mapper.index_down) + "!index");
            }
            else
                ipc->send_udp_message(child_module_name, "hide_cursor_index");

            ipc->send_udp_message(child_module_name, "hide_cursor_thumb");
        }
        else
        {
            ipc->send_udp_message(child_module_name, to_string(pointer_mapper.pt_pinch_to_zoom_index.x) + "!" +
                                                     to_string(pointer_mapper.pt_pinch_to_zoom_index.y) + "!0!1!index");

            ipc->send_udp_message(child_module_name, to_string(pointer_mapper.pt_pinch_to_zoom_thumb.x) + "!" +
                                                     to_string(pointer_mapper.pt_pinch_to_zoom_thumb.y) + "!0!1!thumb");
        }

        ipc->send_udp_message(child_module_name, "update!" + to_string(frame_num));
    }

    ++frame_num;

    if (increment_keypress_count)
    {
        if (keypress_count == calibration_points_count)
        {
            COUT << "step " << calibration_step << " complete" << endl; 

            ipc->send_message("menu_plus", "show calibration next", "");
            ++calibration_step;
        }
        else if (keypress_count < calibration_points_count)
        {
            float percentage = (keypress_count * 100.0 / calibration_points_count);
            COUT << percentage << endl;

            pointer_mapper.add_calibration_point(calibration_step);
        }

        if (calibration_step == 4)
        {
            pointer_mapper.compute_calibration_points();
            calibrating = false;
            increment_keypress_count = false;
            ipc->send_message("menu_plus", "show stage", "");

            COUT << "calibration finished" << endl;
        }

        ++keypress_count;
    }

    if (enable_imshow)
        waitKey(1);
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

void on_key_down(int code)
{
    if (target_pose_name != "")
    {
        if (code == 223)
            record_pose = true;
        else if (code == 49)
            record_pose = false;
    }
    else if (calibrating)
    {
        if (code == calibration_key_codes[calibration_step] && pointer_mapper.active && pose_name == "point")
            increment_keypress_count = true;
    }
    else
        pose_name = "";
}

void on_key_up(int code)
{
    if (calibrating)
    {
        keypress_count = 0;
        increment_keypress_count = false;
        pointer_mapper.reset_calibration(calibration_step);
    }
}

#ifdef _WIN32
HHOOK keyboard_hook_handle;
LRESULT CALLBACK keyboard_handler(int n_code, WPARAM w_param, LPARAM l_param)
{
    KBDLLHOOKSTRUCT* keyboard_hook_stuct = (KBDLLHOOKSTRUCT*)l_param;
    const int code = keyboard_hook_stuct->vkCode;

    if (w_param == WM_KEYDOWN)
        on_key_down(code);
    else if (w_param == WM_KEYUP)
        on_key_up(code);

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

#elif __APPLE__
void udp_receive_thread_function()
{
    sf::UdpSocket local_socket;
    local_socket.bind(32001);

    COUT << "successifvely bound to port 32001" << endl;

    while (true)
    {
        sf::IpAddress sender;
        unsigned short sender_port;
        unsigned char data[255];
        size_t size;
        size_t received;
        local_socket.receive(data, size, received, sender, sender_port);

        string message = string((char*)data);
        if (message != "")
            COUT << message << endl;
    }
}
#endif

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
        else if (str == "set exposure")
        {
            COUT << "please enter exposure value" << endl;

            getline(cin, str);
            camera->setExposureTime(Camera::both, atoi(str.c_str()));

            COUT << "exposure value set to: " + str << endl;
        }
    }
}

void guardian_thread_function()
{
    while (true)
    {
        if (wait_count >= (serial_verified ? 5 : 10))
        {
            COUT << "restarting" << endl;
            wait_for_device();
        }

        ++wait_count;

        if (child_module_name != "" && child_module_path != "")
        {
            static bool first = true;
            if (first && (process_running(child_module_name + extension0) > 0))
            {
                kill_process(child_module_name + extension0);
                while (process_running(child_module_name + extension0) > 0)
                {
                    COUT << "wait kill" << endl;
                    Sleep(100);
                }
            }
            first = false;
        }

        if (child_module_name != "" && child_module_path != "" && process_running(child_module_name + extension0) == 0)
            create_process(child_module_path, child_module_name + extension0, true);

        // if (process_running("daemon_plus" + extension0) == 0)
            // ipc->send_message("everyone", "exit", "");

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
    thread ipc_thread(ipc_thread_function);

    ipc->map_function("exit", [](const string message_body)
    {
        if (child_module_name != "")
            ipc->send_message(child_module_name, "exit", "");

        ipc->clear();
        
#ifdef __APPLE__
        if (camera != NULL)
            camera->stopVideoStream();
#endif

        COUT << "exit 2" << endl;
        exit(0);
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

    ipc->send_message("menu_plus", "show notification", "Please wait:Initializing Touch+ Software");
    ipc->open_udp_channel("menu_plus");

#ifdef _WIN32
    thread keyboard_hook_thread(keyboard_hook_thread_function);

#elif __APPLE__
    thread udp_receive_thread(udp_receive_thread_function);
#endif

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