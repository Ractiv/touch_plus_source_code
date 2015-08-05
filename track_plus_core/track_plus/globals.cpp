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

extern const string cmd_quote = "\"";

extern string serial_number = "";
extern string executable_path = "";
extern string data_path = "";
extern string data_path_current_module = "";
extern string settings_file_path = "";
extern string menu_file_path = "";
extern string ipc_path = "";
extern string pose_database_path = "";

extern string pose_name = "";
extern string target_pose_name = "";
extern string child_module_name = "";
extern string child_module_path = "";
extern string mode = "";

extern bool play = true;
extern bool enable_imshow = true;
extern bool record_pose = false;
extern bool pinch_to_zoom = false;
extern bool has_plate = false;
extern bool has_pen = false;
extern bool motion_detected = false;

extern int actuate_dist_raw = 7;
extern int actuate_dist = actuate_dist_raw;
extern int motion_state = -1;