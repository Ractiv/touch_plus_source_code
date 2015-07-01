#pragma once

#include <string>

using namespace std;

#define COUT cout
// #define COUT ostream(0).flush()

extern const double scale_factor;
extern const int image_width_full;
extern const int image_height_full;
extern const int image_width_small;
extern const int image_height_small;

extern bool manual_corners;

extern unsigned char refresh_delay;
const extern unsigned char initial_delay_count_total;

const extern double fov_width;
const extern double fov_depth;

const extern string cmd_quote;

extern string executable_path;
extern string data_path;
extern string settings_file_path;
extern string ipc_path;