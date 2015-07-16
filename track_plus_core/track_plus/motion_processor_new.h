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

	int vector_completion_size = 5;

	int y_separator_motion_down_median = -1;
	int y_separator_motion_up_median = -1;
	int x_separator_middle = -1;
	int x_separator_middle_median = -1;
	int x_separator_motion_left_median = 0;
	int x_separator_motion_right_median = WIDTH_SMALL;

	uchar gray_threshold = 255;
	uchar gray_threshold_left = 255;
	uchar gray_threshold_right = 255;
	uchar diff_threshold = 255;

	Mat image_background_static = Mat(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1, Scalar(255));

	ValueStore value_store;

	bool compute(Mat& image_in, const string name, const bool visualize);
	bool compute_y_separator_motion();
	bool compute_x_separator_middle();
	bool compute_x_separator_motion_left_right();
	inline void fill_image_background_static(const int x, const int y, Mat& image_in);

private:
	struct compare_point_x
	{
		bool operator() (const Point& pt0, const Point& pt1)
		{
			return (pt0.x < pt1.x);
		}
	};
};