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

Mat image_blob_dilated0 = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
Mat image_blob_dilated1 = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
uchar blob_fill_gray = 0;

int BlobNew::compute_overlap(BlobNew& blob_in, const int x_diff_in, const int y_diff_in, const int dilate_num)
{
	++blob_fill_gray;
	if (blob_fill_gray == 255)
	{
		image_blob_dilated0 = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
		image_blob_dilated1 = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
		blob_fill_gray = 1;
	}

	fill(image_blob_dilated0, blob_fill_gray);
	dilate(image_blob_dilated0, image_blob_dilated0, Mat(), Point(-1, -1), dilate_num);

	blob_in.fill(image_blob_dilated1, blob_fill_gray);
	dilate(image_blob_dilated1, image_blob_dilated1, Mat(), Point(-1, -1), dilate_num);

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
			if (image_blob_dilated0.ptr<uchar>(j, i)[0] == blob_fill_gray)
			{
				const int j_shifted = j + y_diff_in;
				const int i_shifted = i + x_diff_in;

				if (j_shifted > HEIGHT_SMALL_MINUS || i_shifted > WIDTH_SMALL_MINUS || j_shifted < 0 || i_shifted < 0)
					continue;

				if (image_blob_dilated1.ptr<uchar>(j_shifted, i_shifted)[0] == blob_fill_gray)
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