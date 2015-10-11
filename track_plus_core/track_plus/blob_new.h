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
	int index = -1;

	bool active = true;

	Point pt_y_min = Point(0, 9999);
	Point pt_y_max = Point(0, 0);
	Point pt_x_min = Point(9999, 0);
	Point pt_x_max = Point(0, 0);
	Point pt_tip;

	string name = "";

	BlobNew();
	BlobNew(Mat& image_atlas_in, const ushort atlas_id_in);

	void add(const int i, const int j);
	void compute();
	int compute_overlap(BlobNew& blob_in);
	int compute_overlap(BlobNew& blob_in, const int x_diff_in, const int y_diff_in, const int dilate_num);
	float compute_min_dist(Point pt_in, Point* pt_out, bool accurate);
	Point compute_median_point();
	void fill(Mat& image_in, const uchar gray_in, bool check_bounds = false);
};