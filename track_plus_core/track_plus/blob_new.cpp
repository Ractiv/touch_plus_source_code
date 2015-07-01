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

#include "blob_new.h"

BlobNew::BlobNew(){}

BlobNew::BlobNew(Mat& image_atlas_in, const ushort atlas_id_in)
{
	image_atlas = image_atlas_in;
	atlas_id = atlas_id_in;
}

void BlobNew::add(const int i_in, const int j_in)
{
	data.push_back(Point(i_in, j_in));

	if (i_in < x_min)
	{
		x_min = i_in;
		pt_x_min = Point(i_in, j_in);
	}
	if (i_in > x_max)
	{
		x_max = i_in;
		pt_x_max = Point(i_in, j_in);
	}
	if (j_in < y_min)
	{
		y_min = j_in;
		pt_y_min = Point(i_in, j_in);
	}
	if (j_in > y_max)
	{
		y_max = j_in;
		pt_y_max = Point(i_in, j_in);
	}

	image_atlas.ptr<ushort>(j_in, i_in)[0] = atlas_id;
}

void BlobNew::compute()
{
	width = x_max - x_min;
	height = y_max - y_min;
	area = width * height;
	x = (x_max - x_min) / 2 + x_min;
	y = (y_max - y_min) / 2 + y_min;
	count = data.size();
}

int BlobNew::compute_overlap(BlobNew& blob_in)
{
	int overlap_count = 0;
	for (Point& pt : blob_in.data)
		if (image_atlas.ptr<ushort>(pt.y, pt.x)[0] == atlas_id)
			++overlap_count;

	return overlap_count;
}

int BlobNew::compute_overlap(BlobNew& blob_in, const int x_diff_in, const int y_diff_in, const int dilate_num)
{
	Mat image_dilated0 = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
	fill(image_dilated0, 254);
	dilate(image_dilated0, image_dilated0, Mat(), Point(-1, -1), dilate_num);

	Mat image_dilated1 = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
	blob_in.fill(image_dilated1, 254);
	dilate(image_dilated1, image_dilated1, Mat(), Point(-1, -1), dilate_num);

	int i_min = x_min - dilate_num;
	if (i_min < 0)
		i_min = 0;

	int i_max = x_max + dilate_num;
	if (i_max > WIDTH_SMALL_MINUS)
		i_max = WIDTH_SMALL_MINUS;

	int j_min = y_min - dilate_num;
	if (j_min < 0)
		j_min = 0;

	int j_max = y_max + dilate_num;
	if (j_max > HEIGHT_SMALL_MINUS)
		j_max = HEIGHT_SMALL_MINUS;

	const int i_max_const = i_max;
	const int j_max_const = j_max;

	int overlap_count = 0;
	for (int i = i_min; i <= i_max_const; ++i)
		for (int j = j_min; j <= j_max_const; ++j)
			if (image_dilated0.ptr<uchar>(j, i)[0] == 254)
			{
				const int j_shifted = j + y_diff_in;
				const int i_shifted = i + x_diff_in;

				if (j_shifted > HEIGHT_SMALL_MINUS || i_shifted > WIDTH_SMALL_MINUS)
					continue;

				if (image_dilated1.ptr<uchar>(j_shifted, i_shifted)[0] == 254)
					++overlap_count;
			}

	return overlap_count;
}

float BlobNew::compute_min_dist(Point pt_in)
{
	float dist_min = 9999;
	for (Point& pt : data)
	{
		const float dist_current = get_distance(pt_in, pt);
		if (dist_current < dist_min)
			dist_min = dist_current;
	}
	return dist_min;
}

Point BlobNew::compute_median_point()
{
	vector<int> x_vec;
	vector<int> y_vec;
	for (Point& pt : data)
	{
		x_vec.push_back(pt.x);
		y_vec.push_back(pt.y);
	}
	sort(x_vec.begin(), x_vec.end());
	sort(y_vec.begin(), y_vec.end());
	
	return Point(x_vec[x_vec.size() / 2], y_vec[y_vec.size() / 2]);
}

void BlobNew::fill(Mat& image_in, const uchar gray_in)
{
	for (Point pt : data)
		image_in.ptr<uchar>(pt.y, pt.x)[0] = gray_in;
}