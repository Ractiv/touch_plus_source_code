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
#include <iostream>

using namespace std;

#ifdef __APPLE__
#include <unistd.h>
#define Sleep(a) usleep(a * 1000)
#define BYTE unsigned char*
#endif

#define SHOW_CONSOLE

extern const double scale_factor;
extern const int image_width_full;
extern const int image_height_full;
extern const int image_width_small;
extern const int image_height_small;

extern bool manual_corners;

extern unsigned char refresh_delay;
const extern unsigned char initial_delay_count_total;

const extern string cmd_quote;
const extern string slash;
const extern string extension0;
const extern string extension1;

extern string executable_path;
extern string data_path;
extern string settings_file_path;
extern string ipc_path;