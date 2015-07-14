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

#include "value_store.h"

class ToolMonoProcessor
{
public:	
	vector<BlobNew*> blob_vec;

	Point pt_motion_center;
	Point pt_led_center;
	Point pt_led0;
	Point pt_led1;
	Point pt_led2;
	Point pt_led3;

	float radius;

	int x_min0;
	int x_max0;
	int y_min0;
	int y_max0;

	int x_min1;
	int x_max1;
	int y_min1;
	int y_max1;

	int x_min2;
	int x_max2;
	int y_min2;
	int y_max2;

	int x_min3;
	int x_max3;
	int y_min3;
	int y_max3;

	ValueStore value_store;
	Mat image_background_static = Mat(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1, Scalar(255));

	bool compute(Mat& image_active_light_in, Mat& image_preprocessed_in, const string name);
	inline void fill_image_background_static(const int x, const int y, Mat& image_in);
};