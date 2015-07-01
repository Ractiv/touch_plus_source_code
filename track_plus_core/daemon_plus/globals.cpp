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

extern const double scale_factor = 0.25;
extern const int image_width_full = 640;
extern const int image_height_full = 480;
extern const int image_width_small = image_width_full * scale_factor;
extern const int image_height_small = image_height_full * scale_factor;

extern const double fov_width = 23.7;
extern const double fov_depth = 17.9;

extern const string cmd_quote = "\"";

extern string executable_path = "";
extern string data_path = "";
extern string settings_file_path = "";
extern string ipc_path = "";