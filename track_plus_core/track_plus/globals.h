#pragma once

#include <string>

using namespace std;

#define WIDTH_LARGE        640
#define HEIGHT_LARGE       480
#define WIDTH_LARGE_MINUS  639
#define HEIGHT_LARGE_MINUS 479
#define WIDTH_SMALL        160
#define HEIGHT_SMALL       120
#define REFLECTION_Y       90
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

extern bool play;
extern bool enable_imshow;
extern bool record_pose;
extern bool pinch_to_zoom;

extern int actuate_dist_raw;
extern int actuate_dist;