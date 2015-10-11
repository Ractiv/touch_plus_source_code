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
#include "ipc.h"

using namespace std;
using namespace cv;

class Reprojector
{
public:
	double a_out;
	double b_out;
	double c_out;
	double d_out;

	Point** rect_mat0 = NULL;
	Point** rect_mat1 = NULL;

	int y_top0;
	int y_top1;
	int y_bottom0;
	int y_bottom1;

	void load(IPC& ipc, bool flipped);
	void proceed();
	float compute_depth(float disparity_in);
	Point2f compute_plane_size(float depth);
	Point3f reproject_to_3d(float pt0_x, float pt0_y, float pt1_x, float pt1_y);
	Mat remap(Mat* const image_in, const uchar side, const bool interpolate);
	Mat remap(Mat* const image_in, const int x_offset, const int y_offset, const uchar side, Point& pt_offset);
	Point remap_point(Point& pt_in, const uchar side, const uchar scale);
	void compute_y_bounds();
	void y_align(Mat& image0, Mat& image1, bool interpolate);
};