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
#include "value_store.h"
#include "value_accumulator.h"

class MotionProcessorNew
{
public:
	string algo_name = "motion_processor";

	int entropy_threshold = 300;

	float y_separator_down = HEIGHT_SMALL_MINUS;
	float y_separator_up = 0;
	float x_separator_middle = WIDTH_SMALL_HALF;
	float x_separator_left = 0;
	float x_separator_right = WIDTH_SMALL;

	float y_separator_down_median = HEIGHT_SMALL_MINUS;
	float y_separator_up_median = 0;
	float x_separator_middle_median = WIDTH_SMALL_HALF;
	float x_separator_left_median = 0;
	float x_separator_right_median = WIDTH_SMALL;

	float gray_threshold_left = 9999;
	float gray_threshold_right = 9999;
	float diff_threshold = 9999;

	int completion_count = 0;

	bool will_compute_next_frame = false;

	bool compute_background_static = false;
	bool compute_x_separator_middle = true;

	bool both_moving;
	bool left_moving;
	bool right_moving;

	Mat image_background_static = Mat(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1, Scalar(255));

	ValueStore* value_store = new ValueStore();

	ValueAccumulator value_accumulator;

	bool compute(Mat& image_in,             Mat& image_raw, const int y_ref, float pitch,
				 bool construct_background, string name,    bool visualize);

	inline void fill_image_background_static(const int x, const int y, Mat& image_in);
	Mat compute_image_foreground(Mat& image_in);
};