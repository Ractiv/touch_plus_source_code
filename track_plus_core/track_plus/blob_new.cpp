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
	const int i_max = blob_in.x_max + dilate_num + 10 + x_diff_in;
	const int j_max = blob_in.y_max + dilate_num + 10 + y_diff_in;

	if (i_max < 1 || j_max < 1)
		return 0;

	Mat image_blob_in = Mat::zeros(j_max, i_max, CV_8UC1);
	for (Point& pt : blob_in.data)
	{
		int i = pt.x + x_diff_in;
		int j = pt.y + y_diff_in;

		if (i < 0 || i >= i_max || j < 0 || j >= j_max)
			continue;

		image_blob_in.ptr<uchar>(j, i)[0] = 254;
	}

	dilate(image_blob_in, image_blob_in, Mat(), Point(-1, -1), dilate_num);

	int overlap_count = 0;
	for (Point& pt : data)
	{
		if (pt.x >= i_max || pt.y >= j_max)
			continue;
		
		if (image_blob_in.ptr<uchar>(pt.y, pt.x)[0] > 0)
			++overlap_count;
	}

	return overlap_count;
}

float BlobNew::compute_min_dist(Point pt_in, Point* pt_out, bool accurate)
{
	float dist_min = 9999;
	Point pt_dist_min;
	for (Point& pt : data)
	{
		const float dist_current = get_distance(pt_in, pt, accurate);
		if (dist_current < dist_min)
		{
			dist_min = dist_current;
			pt_dist_min = pt;
		}
	}
	if (pt_out != NULL)
		*pt_out = pt_dist_min;

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

void BlobNew::fill(Mat& image_in, const uchar gray_in, bool check_bounds)
{
	if (!check_bounds)
	{
		for (Point pt : data)
			image_in.ptr<uchar>(pt.y, pt.x)[0] = gray_in;
	}
	else
	{
		const int image_width = image_in.cols;
		const int image_height = image_in.rows;

		for (Point pt : data)
			if (pt.x >= 0 && pt.y >= 0 && pt.x < image_width && pt.y < image_height)
				image_in.ptr<uchar>(pt.y, pt.x)[0] = gray_in;
	}
}