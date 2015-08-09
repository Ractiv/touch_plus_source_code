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

	float y_separator_down = HEIGHT_SMALL_MINUS;
	float y_separator_up = 0;
	float x_separator_middle = WIDTH_SMALL / 2;
	float x_separator_left = 0;
	float x_separator_right = WIDTH_SMALL;

	float gray_threshold_left = 9999;
	float gray_threshold_right = 9999;
	float diff_threshold = 9999;

	bool compute_background_static = false;

	bool left_hand_is_moving = false;
	bool right_hand_is_moving = false;
	bool both_hands_are_moving = false;

	Mat image_background_static = Mat(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1, Scalar(255));

	ValueStore value_store;

	bool compute(Mat& image_in, Mat& image_raw_in, bool construct_background, const string name, const bool visualize);
	inline void fill_image_background_static(const int x, const int y, Mat& image_in);
	Mat compute_image_foreground(Mat& image_in);
};