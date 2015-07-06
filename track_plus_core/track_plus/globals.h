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

#pragma once

#include <string>

using namespace std;

#define WIDTH_LARGE        640
#define HEIGHT_LARGE       480
#define WIDTH_LARGE_MINUS  639
#define HEIGHT_LARGE_MINUS 479
#define WIDTH_SMALL        160
#define HEIGHT_SMALL       120
#define REFLECTION_Y       70
#define WIDTH_SMALL_MINUS  159
#define HEIGHT_SMALL_MINUS 119
#define WIDTH_MIN          80
#define HEIGHT_MIN         60

#define FOV_WIDTH          30
// #define FOV_WIDTH          23.7
#define FOV_DEPTH          17.9

#define COUT cout
// #define COUT ostream(0).flush()

extern const string cmd_quote;

extern string serial_number;
extern string executable_path;
extern string data_path;
extern string data_path_current_module;
extern string settings_file_path;
extern string menu_file_path;
extern string ipc_path;
extern string pose_database_path;

extern string pose_name;
extern string target_pose_name;
extern string child_module_name;
extern string child_module_path;
extern string mode;

extern bool play;
extern bool enable_imshow;
extern bool record_pose;
extern bool pinch_to_zoom;

extern int actuate_dist_raw;
extern int actuate_dist;