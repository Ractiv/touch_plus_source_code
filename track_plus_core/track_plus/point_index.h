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

struct compare_point_index_index
{
	bool operator() (const PointIndex& pt_index0, const PointIndex& pt_index1)
	{
		return (pt_index0.index < pt_index1.index);
	}
};