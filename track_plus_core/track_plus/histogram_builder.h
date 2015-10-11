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
#include "globals.h"
#include "math_plus.h"

using namespace cv;

class HistogramBuilder
{
public:
	void compute_vertical(Mat image_in, Mat image_out, int gaussian_val, int& x_min, int& x_max, int& y_min, int& y_max);
	void compute_vertical(vector<int>& vec, Mat image_out, int gaussian_val, int& x_min, int& x_max, int& y_min, int& y_max);
	void compute_horizontal(Mat image_in, Mat image_out, int gaussian_val, int& x_min, int& x_max, int& y_min, int& y_max);
	void compute_horizontal(vector<int>& vec, Mat image_out, int gaussian_val, int& x_min, int& x_max, int& y_min, int& y_max);
};