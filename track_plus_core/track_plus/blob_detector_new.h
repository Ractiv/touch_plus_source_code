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

	void compute(Mat& image_in, uchar gray_in, int x_min_in, int x_max_in, int y_min_in, int y_max_in, bool shallow_copy);
	void compute_location(Mat& image_in, const uchar gray_in, const int i, const int j, bool shallow_copy, bool in_process = false);
	void compute_all(Mat& image_in);
	void sort_blobs_by_count();
	void sort_blobs_by_angle(Point& pivot);
	void sort_blobs_by_x();
	void sort_blobs_by_y_max();
	void sort_blobs_by_x_min();
	void sort_blobs_by_convex_points_y();

private:
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
};