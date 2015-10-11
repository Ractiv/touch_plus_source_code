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

#include "math_plus.h"
#include "contour_functions.h"
#include "opencv2/opencv.hpp"

using namespace cv;
using namespace std;

Mat compute_cost_mat(vector<Point>& vec0, vector<Point>& vec1, bool stereo);
float compute_dtw(Mat cost_mat);
vector<Point> compute_dtw_indexes(Mat cost_mat);