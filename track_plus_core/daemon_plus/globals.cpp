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