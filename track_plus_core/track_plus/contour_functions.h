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

using namespace std;
using namespace cv;

struct compare_index_point_x
{
	vector<Point>* pt_vec; 

	compare_index_point_x(vector<Point>* pt_vec_in)
	{
		pt_vec = pt_vec_in;
	}

	bool operator() (const int index0, const int index1)
	{
		const int x0 = (*pt_vec)[index0].x;
		const int x1 = (*pt_vec)[index1].x;

		return x0 < x1;
	}
};

struct PointIndex
{
	Point pt;
	int index;

	PointIndex(Point& pt_in, int index_in)
	{
		pt = pt_in;
		index = index_in;
	}

	bool operator== (const PointIndex& point1)
	{
		return pt == point1.pt;
	}
};

struct compare_point_index_index
{
	bool operator() (const PointIndex& pt_index0, const PointIndex& pt_index1)
	{
		return (pt_index0.index < pt_index1.index);
	}
};

void approximate_contour(vector<Point>& points, vector<Point>& points_approximated, int theta_threshold, int skip_count);

void compute_unwrap(vector<Point>& points, Point& pivot, vector<int>& convex_indexes, vector<int>& concave_indexes,
					vector<Point>& points_unwrapped);

void compute_unwrap2(vector<Point>& points, Point& pivot, vector<Point>& points_unwrapped);

void midpoint_circle(int x_in, int y_in, int radius_in, vector<Point>& result_out);

void midpoint_circle_push_pixel(int x, int y, int x_c, int y_c, vector<PointIndex>& result_out,
								int& c00, int& c01, int& c10, int& c11, int& c20, int& c21, int& c30, int& c31);