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

#include "blob_new.h"
#include "globals.h"
#include "math_plus.h"

class BlobDetectorNew
{
public:
	vector<BlobNew>* blobs = new vector<BlobNew>();
	BlobNew blob_max_size_actual;
	BlobNew* blob_max_size = &blob_max_size_actual;

	Mat image_atlas;

	int x_min_result;
	int x_max_result;
	int y_min_result;
	int y_max_result;

	Point pt_y_max_result;

	void compute(Mat& image_in, uchar gray_in, int x_min_in, int x_max_in, int y_min_in, int y_max_in, bool shallow, bool octal = 0);
	void compute_region(Mat& image_in, uchar gray_in, vector<Point>& region_vec, bool shallow, bool octal);
	void compute_location(Mat& image_in, const uchar gray_in, const int i, const int j, bool shallow, bool in_process = 0, bool octal = 0);
	void compute_all(Mat& image_in, bool octal);
	void sort_blobs_by_count();
	void sort_blobs_by_angle(Point& pivot);
	void sort_blobs_by_x();
	void sort_blobs_by_y_max();
	void sort_blobs_by_x_min();
};