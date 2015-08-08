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

	int y_separator_down = -1;
	int y_separator_up = 0;
	int x_separator_middle = WIDTH_SMALL / 2;
	int x_separator_left = 0;
	int x_separator_right = WIDTH_SMALL;

	float gray_threshold_left = 9999;
	float gray_threshold_right = 9999;
	float diff_threshold = 9999;

	bool compute_y_separator_down = true;

	Mat image_background_static = Mat(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1, Scalar(255));

	ValueStore value_store;

	bool compute(Mat& image_in, Mat& image_raw_in, const string name, const bool visualize);
	inline void fill_image_background_static(const int x, const int y, Mat& image_in);
	Mat compute_image_foreground(Mat& image_in);
};