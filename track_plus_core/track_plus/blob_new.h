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

class BlobNew
{
public:
	Mat image_atlas;

	ushort atlas_id;

	vector<Point> data;
	vector<Point> tips;

	vector<int> index_vec;

	vector<vector<Point>> extension_vecs;

	vector<Point> convex_points;

	vector<string> names;

	int x_min = 9999;
	int x_max = 0;
	int y_min = 9999;
	int y_max = 0;
	int width;
	int height;
	int area;
	int count = 0;
	int x;
	int y;
	int index;
	int id = -1;

	float dist = 0;

	int* id_ptr = NULL;

	bool active = true;
	bool merge = false;
	bool selected = false;

	Point pt_y_min = Point(0, 9999);
	Point pt_y_max = Point(0, 0);
	Point pt_x_min = Point(9999, 0);
	Point pt_x_max = Point(0, 0);

	BlobNew* matching_blob = NULL;

	string name = "";

	BlobNew();
	BlobNew(Mat& image_atlas_in, const ushort atlas_id_in);

	void add(const int i, const int j);
	void compute();
	int compute_overlap(BlobNew& blob_in);
	int compute_overlap(BlobNew& blob_in, const int x_diff_in, const int y_diff_in, const int dilate_num);
	float compute_min_dist(Point pt_in);
	Point compute_median_point();
	void fill(Mat& image_in, const uchar gray_in);
};

struct compare_blob_count
{
	bool operator() (const BlobNew& blob0, const BlobNew& blob1)
	{
		return (blob0.count > blob1.count);
	}
};

struct compare_blob_angle
{
	Point pivot;

	compare_blob_angle(Point& pivot_in)
	{
		pivot = pivot_in;
	}

	bool operator() (const BlobNew& blob0, const BlobNew& blob1)
	{
		float theta0 = get_angle(blob0.x, blob0.y, pivot.x, pivot.y);
		float theta1 = get_angle(blob1.x, blob1.y, pivot.x, pivot.y);

		return theta0 > theta1;
	}
};

struct compare_blob_dist
{
	bool operator() (const BlobNew& blob0, const BlobNew& blob1)
	{
		return blob0.dist < blob1.dist;
	}
};

struct compare_blob_x
{
	bool operator() (const BlobNew& blob0, const BlobNew& blob1)
	{
		return (blob0.x < blob1.x);
	}
};

struct compare_blob_y_max
{
	bool operator() (const BlobNew& blob0, const BlobNew& blob1)
	{
		return (blob0.y_max > blob1.y_max);
	}
};

struct compare_blob_x_min
{
	bool operator() (const BlobNew& blob0, const BlobNew& blob1)
	{
		return (blob0.x_min < blob1.x_min);
	}
};

struct compare_blob_convex_points_y
{
	bool operator() (const BlobNew& blob0, const BlobNew& blob1)
	{
		int y_max0 = 0;
		for (Point pt : (blob0.convex_points))
			if (pt.y > y_max0)
				y_max0 = pt.y;

		int y_max1 = 0;
		for (Point pt : (blob1.convex_points))
			if (pt.y > y_max1)
				y_max1 = pt.y;

		return (y_max0 > y_max1);
	}
};