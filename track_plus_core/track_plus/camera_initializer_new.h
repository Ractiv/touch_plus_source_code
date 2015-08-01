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

#include <opencv2/opencv.hpp>
#include "camera.h"
#include "math_plus.h"
#include "globals.h"
#include "mat_functions.h"
#include "low_pass_filter.h"

using namespace cv;

class CameraInitializerNew
{
public:
	static float exposure_val;
	static float exposure_max;

	static uchar l_exposure_old;

	static LowPassFilter low_pass_filter;
	
	static void init(Camera* camera);
	static bool adjust_exposure(Camera* camera, Mat& image_in);
	static float exponential(float x);
	static float linear(float x);
	static void preset0(Camera* camera);
	static void preset1(Camera* camera);
	static void preset2(Camera* camera);
	static void preset3(Camera* camera);
	static void preset4(Camera* camera);
};