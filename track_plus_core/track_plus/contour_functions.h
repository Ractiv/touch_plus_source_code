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
#include "point_index.h"

using namespace std;
using namespace cv;

vector<vector<Point>> legacyFindContours(Mat& Segmented);

void approximate_contour(vector<Point>& points, vector<Point>& points_approximated, int theta_threshold, int skip_count);

void compute_unwrap(vector<Point>& points, Point pivot, vector<int>& convex_indexes, vector<int>& concave_indexes,
					vector<Point>& points_unwrapped);

void compute_unwrap2(vector<Point>& points, Point pivot, vector<Point>& points_unwrapped);

void midpoint_circle(int x_in, int y_in, int radius_in, vector<Point>& result_out);

void bresenham_line(int x1_in, int y1_in, int const x2_in, int const y2_in, vector<Point>& result_out, const uchar count_threshold);

void extension_line(Point pt_start, Point pt_end, const uchar length, vector<Point>& line_points, const bool reverse);

Point get_y_min_point(vector<Point>& pt_vec);

Point get_y_max_point(vector<Point>& pt_vec);

Point get_x_min_point(vector<Point>& pt_vec);

Point get_x_max_point(vector<Point>& pt_vec);

void get_bounds(vector<Point>& pt_vec, int& x_min, int& x_max, int& y_min, int& y_max);

bool check_bounds_small(Point& pt);