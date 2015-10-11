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

#include "histogram_builder.h"

void HistogramBuilder::compute_vertical(Mat& image_in, Mat& image_out, int gaussian_val, int& x_min, int& x_max, int& y_min, int& y_max)
{
	x_min = 0;
	x_max = 0;
	y_min = 9999;
	y_max = 0;

	const int height_small = image_in.rows;
	const int width_small = image_in.cols;

	image_out = Mat::zeros(height_small, width_small, CV_8UC1);

	for (int j = 0; j < height_small; ++j)
	{
		int count = 0;
		for (int i = 0; i < width_small; ++i)
			if (image_in.ptr<uchar>(j, i)[0] > 0)
				++count;

		if (count > 0)
		{
			line(image_out, Point(0, j), Point(count, j), Scalar(254), 1);

			if (count > x_max)
				x_max = count;
			if (j < y_min)
				y_min = j;
			if (j > y_max)
				y_max = j;
		}
	}
	GaussianBlur(image_out, image_out, Size(gaussian_val, gaussian_val), 0, 0);
	threshold(image_out, image_out, 150, 254, THRESH_BINARY);
}

void HistogramBuilder::compute_vertical(vector<int>& vec, Mat& image_out, int gaussian_val, int& x_min, int& x_max, int& y_min, int& y_max)
{
	x_min = 0;
	x_max = 0;
	y_min = 9999;
	y_max = 0;

	const int height_small = *max_element(vec.begin(), vec.end()) + 1;
	const int width_small = vec.size();

	image_out = Mat::zeros(height_small, width_small, CV_8UC1);

	const int vec_size = vec.size();
	for (int j = 0; j < vec_size; ++j)
	{
		int count = vec[j];
		if (count > 0)
		{
			line(image_out, Point(0, j), Point(count, j), Scalar(254), 1);

			if (count > x_max)
				x_max = count;
			if (j < y_min)
				y_min = j;
			if (j > y_max)
				y_max = j;
		}
	}
	GaussianBlur(image_out, image_out, Size(gaussian_val, gaussian_val), 0, 0);
	threshold(image_out, image_out, 150, 254, THRESH_BINARY);
}

void HistogramBuilder::compute_horizontal(Mat& image_in, Mat& image_out, int gaussian_val, int& x_min, int& x_max, int& y_min, int& y_max)
{
	x_min = 9999;
	x_max = 0;
	y_min = 0;
	y_max = 0;

	const int height_small = image_in.rows;
	const int width_small = image_in.cols;
	
	image_out = Mat::zeros(height_small, width_small, CV_8UC1);

	for (int i = 0; i < width_small; ++i)
	{
		int count = 0;
		for (int j = 0; j < height_small; ++j)
			if (image_in.ptr<uchar>(j, i)[0] > 0)
				++count;

		if (count > 0)
		{
			line(image_out, Point(i, 0), Point(i, count), Scalar(254), 1);

			if (count > y_max)
				y_max = count;
			if (i < x_min)
				x_min = i;
			if (i > x_max)
				x_max = i;
		}
	}
	GaussianBlur(image_out, image_out, Size(gaussian_val, gaussian_val), 0, 0);
	threshold(image_out, image_out, 150, 254, THRESH_BINARY);
}

void HistogramBuilder::compute_horizontal(vector<int>& vec, Mat& image_out, int gaussian_val, int& x_min, int& x_max, int& y_min, int& y_max)
{
	x_min = 9999;
	x_max = 0;
	y_min = 0;
	y_max = 0;

	const int height_small = vec.size();
	const int width_small = *max_element(vec.begin(), vec.end()) + 1;
	
	image_out = Mat::zeros(height_small, width_small, CV_8UC1);

	const int vec_size = vec.size();
	for (int i = 0; i < vec_size; ++i)
	{
		int count = vec[i];
		if (count > 0)
		{
			line(image_out, Point(i, 0), Point(i, count), Scalar(254), 1);

			if (count > y_max)
				y_max = count;
			if (i < x_min)
				x_min = i;
			if (i > x_max)
				x_max = i;
		}
	}
	GaussianBlur(image_out, image_out, Size(gaussian_val, gaussian_val), 0, 0);
	threshold(image_out, image_out, 150, 254, THRESH_BINARY);
}