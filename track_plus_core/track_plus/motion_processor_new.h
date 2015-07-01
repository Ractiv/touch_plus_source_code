#pragma once

#include "globals.h"
#include "blob_detector_new.h"
#include "low_pass_filter.h"
#include "contour_functions.h"
#include "mat_functions.h"
#include "value_store.h"

class MotionProcessorNew
{
public:
	int noise_size = 0;

	int y_separator_motion_down_median = -1;
	int y_separator_motion_up_median = -1;
	int x_separator_middle = -1;
	int x_separator_middle_median = -1;
	int x_separator_motion_left_median = 0;
	int x_separator_motion_right_median = WIDTH_SMALL;

	uchar gray_threshold = 255;
	uchar diff_threshold = 255;

	Mat image_background_static = Mat(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1, Scalar(255));

	ValueStore value_store;

	bool compute(Mat& image_in, const string name, const bool visualize);
	bool compute_y_separator_motion();
	bool compute_x_separator_middle();
	bool compute_x_separator_motion_left_right();
	inline void fill_image_background_static(const int x, const int y, Mat& image_in);
	void compute_slope(Mat& image_in_thresholded, string name);

private:
	struct compare_point_x
	{
		bool operator() (const Point& pt0, const Point& pt1)
		{
			return (pt0.x < pt1.x);
		}
	};
};