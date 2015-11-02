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

#include "globals.h"

const string cmd_quote = "\"";

#ifdef _WIN32
const string slash = "\\";
const string extension0 = ".exe";
const string extension1 = ".exe";

#elif __APPLE__
const string slash = "/";
const string extension0 = "";
const string extension1 = ".app";
#endif

const string module_name = "track_plus";

string serial_number = "";
string executable_path = "";
string data_path = "";
string data_path_current_module = "";
string settings_file_path = "";
string ipc_path = "";
string processes_temp_path = "";
string pose_database_path = "";

string pose_name = "";
string target_pose_name = "";
string child_module_name = "";
string child_module_path = "";

bool play = true;
bool enable_imshow = true;
bool record_pose = false;
bool pinch_to_zoom = false;
bool has_plate = false;
bool has_pen = false;
bool motion_detected = false;

int actuate_dist_raw = 8;
int actuate_dist = actuate_dist_raw;

vector<string> algo_name_vec = vector<string>();
vector<string> algo_name_vec_old = vector<string>();