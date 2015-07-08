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

#include "value_store.h"
#include "c_tracker.h"
#include "permutation.h"
#include "contour_functions.h"

struct Trace2D
{
	float x;
	float y;
	int id;
	int size;
	int size_total;
	Point2f point;
	vector<Point2f> points;
	float dist;

	Trace2D(Point2f& point_in, int id_in, vector<Point2f>& points_in, float& dist_in, int size_total_in)
	{
		x = point_in.x;
		y = point_in.y;
		point = point_in;
		id = id_in;
		points = points_in;
		dist = dist_in;
		size = points_in.size();
		size_total = size_total_in;
	}
};

class ToolTrackerMonoFrame
{
private:
	struct compare_trace_dist_size_ratio
	{
		bool operator() (const Trace2D& trace0, const Trace2D& trace1)
		{
			float ratio0 = trace0.dist / trace0.size_total;
			float ratio1 = trace1.dist / trace1.size_total;

			return ratio0 > ratio1;
		}
	};

public:
	vector<Trace2D> trace_vec;

	void sort_traces_by_dist_size_ratio()
	{
		sort(trace_vec.begin(), trace_vec.end(), compare_trace_dist_size_ratio());
	}
};

struct Rect2D
{
	Point pt0;
	Point pt1;
	Point pt2;
	Point pt3;
	float ratio;

	Rect2D(Point& pt0_in, Point& pt1_in, Point& pt2_in, Point& pt3_in, float& ratio_in)
	{
		pt0 = pt0_in;
		pt1 = pt1_in;
		pt2 = pt2_in;
		pt3 = pt3_in;
		ratio = ratio_in;
	}
};

class ToolTrackerMonoProcessor
{
public:
	CTracker tracker = CTracker(0.2, 0.5, 9999, 0, 10);
	ValueStore value_store;

	Mat image_background_static = Mat(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1, Scalar(255));

	const int cache_num = 10;
	const int scan_num = 2;
	const int permutation_k = 6;
	int track_id_max = -1;

	vector<ToolTrackerMonoFrame> frame_vec;

	void compute(Mat& image_active_light_in, Mat& image_preprocessed_in, const string name);
	ToolTrackerMonoFrame get_cached_frame(const int past_frame_num);
	inline void fill_image_background_static(const int x, const int y, Mat& image_in);


private:
	struct compare_blob_dist_to_pt
	{
		Point pt;

		compare_blob_dist_to_pt(Point& pt_in)
		{
			pt = pt_in;
		}

		bool operator() (const BlobNew* blob0, const BlobNew* blob1)
		{
			float dist0 = get_distance(blob0->x, blob0->y, pt.x, pt.y);
			float dist1 = get_distance(blob1->x, blob1->y, pt.x, pt.y);

			return dist0 < dist1;
		}
	};

	struct compare_blob_angle
	{
		Point pivot;

		compare_blob_angle(Point& pivot_in)
		{
			pivot = pivot_in;
		}

		bool operator() (const BlobNew* blob0, const BlobNew* blob1)
		{
			float theta0 = get_angle(blob0->x, blob0->y, pivot.x, pivot.y);
			float theta1 = get_angle(blob1->x, blob1->y, pivot.x, pivot.y);

			return theta0 > theta1;
		}
	};
};