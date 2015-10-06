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

#include "contour_functions.h"

vector<vector<Point>> legacyFindContours(Mat& Segmented)
{
	static const int thickness = 1;
	static const int lineType = 8;

	IplImage        SegmentedIpl = Segmented;
	CvMemStorage*   storage = cvCreateMemStorage(0);
	CvSeq*          contours = 0;
	int             numCont = 0;
	int             contAthresh = 45;

	numCont = cvFindContours(&SegmentedIpl, storage, &contours, sizeof(CvContour), CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, cvPoint(0, 0));

	vector<vector<Point>> result;
	for (; contours != 0; contours = contours->h_next)
	{
		vector<Point> contour_current;

		Point pt_old = Point(-1, -1);
		for (int i = 0; i < contours->total; ++i)
		{
			CvPoint* point = (CvPoint*)CV_GET_SEQ_ELEM(CvPoint, contours, i);
			Point pt = Point(point->x, point->y);

			if (pt_old.x != -1)
			{
				vector<Point> line_points;
				bresenham_line(pt_old.x, pt_old.y, pt.x, pt.y, line_points, 1000);

				int index = 0;
				for (Point& pt : line_points)
				{
					if (index > 0)
						contour_current.push_back(pt);

					++index;
				}
			}

			pt_old = pt;
		}

		vector<Point> contour_current_reduced;
		for (int i = 0; i < contour_current.size(); i += 1)
			contour_current_reduced.push_back(contour_current[i]);

		result.push_back(contour_current_reduced);
	}

	cvReleaseMemStorage(&storage);
	return result;
}

void approximate_contour(vector<Point>& points, vector<Point>& points_approximated, int theta_threshold, int skip_count)
{
	if (points.size() == 0)
		return;

	points_approximated.push_back(points[0]);

	Point pt_old = Point(-1, 0);
	float angle_old = 9999;

	const int points_size = points.size();

	for (int i = 0; i < points_size; i += skip_count)
	{
		Point pt = points[i];

		if (pt_old.x != -1)
		{
			const float angle = get_angle(pt_old.x, pt_old.y, pt.x, pt.y);

			if (abs(angle - angle_old) > theta_threshold)
			{
				angle_old = angle;
				points_approximated.push_back(pt_old);
			}
		}
		pt_old = pt;
	}

	Point pt_end = points[points.size() - 1];
	Point pt_end_approximated = points_approximated[points_approximated.size() - 1];
	if (pt_end_approximated.x != pt_end.x || pt_end_approximated.y != pt_end.y)
		points_approximated.push_back(pt_end);
}

void compute_unwrap(vector<Point>& points, Point pivot, vector<int>& convex_indexes, vector<int>& concave_indexes,
					vector<Point>& points_unwrapped)
{
	if (points.size() == 0)
		return;

	const int points_size = points.size();

	Mat image_out = Mat::zeros(300, points_size, CV_8UC1);
	int dist_max = 0;
	int dist_min = 9999;

	int index = 0;
	points_unwrapped = vector<Point>(points_size);

	for (Point& pt : points)
	{
		const int dist = get_distance(pt, pivot);

		if (dist > dist_max)
			dist_max = dist;
		if (dist < dist_min)
			dist_min = dist;

		if (index > 0)
		{
			Point pt_current = Point(index, dist);
			line(image_out, pt_current, Point(index, 0), Scalar(254), 1);
			points_unwrapped[index] = pt_current;
		}

		++index;
	}

	const int dist_max_const = dist_max;
	const int dist_min_const = dist_min;
	for (int j = dist_min_const; j < dist_max_const; j += 4)
	{
		if (j >= image_out.rows)
			break;

		bool white_old = false;
		int index0;

		for (int i = 0; i < points_size; ++i)
		{
			if (i >= image_out.cols)
				continue;

			bool white_new = image_out.ptr<uchar>(j, i)[0] > 0;

			if (white_new && !white_old)
				index0 = i;
			else if (!white_new && white_old)
			{
				int y_max = 0;
				int index_y_max;

				const int index1 = i;
				for (int a = index0; a <= index1; ++a)
				{
					int y_current = points_unwrapped[a].y;
					if (y_current > y_max)
					{
						y_max = y_current;
						index_y_max = a;
					}
				}

				bool index_found = false;

				for (int& index_convex : convex_indexes)
					if (index_convex == index_y_max)
						index_found = true;

				if (!index_found)
					convex_indexes.push_back(index_y_max);
			}

			white_old = white_new;
		}
	}

	sort(convex_indexes.begin(), convex_indexes.end(), compare_index_point_x(&points_unwrapped));

	const int convex_indexes_size = convex_indexes.size();
	for (int i = 1; i < convex_indexes_size; ++i)
	{
		const int index0 = convex_indexes[i - 1];
		const int index1 = convex_indexes[i];

		Point pt0 = points_unwrapped[index0];
		Point pt1 = points_unwrapped[index1];

		if (pt0.x == pt1.x && pt0.y == pt1.y)
			continue;

		int y_min = 9999;
		int index_y_min;

		for (int a = index0; a < index1; ++a)
		{
			const int y_current = points_unwrapped[a].y;
			if (y_current < y_min)
			{
				y_min = y_current;
				index_y_min = a;
			}
		}

		bool index_found = false;

		for (int& index_concave : concave_indexes)
			if (index_concave == index_y_min)
				index_found = true;

		if (!index_found)
			concave_indexes.push_back(index_y_min);
	}
}

void compute_unwrap2(vector<Point>& points, Point pivot, vector<Point>& points_unwrapped)
{
	if (points.size() == 0)
		return;

	const int points_size = points.size();

	int dist_max = 0;
	int dist_min = 9999;

	int index = 0;
	int x_current = 0;
	Point pt_old = Point(-1, -1);
	points_unwrapped = vector<Point>(points_size - 1);

	for (Point& pt : points)
	{
		const int dist = get_distance(pt, pivot);

		if (dist > dist_max)
			dist_max = dist;
		if (dist < dist_min)
			dist_min = dist;

		if (index > 0)
		{
			Point pt_current = Point(x_current, dist);
			points_unwrapped[index - 1] = pt_current;
			x_current += get_distance(pt_old, pt);
		}

		++index;
		pt_old = pt;
	}
}

void midpoint_circle_push_pixel(int x, int y, int x_c, int y_c, vector<PointIndex>& result_out,
						        int& c00, int& c01, int& c10, int& c11, int& c20, int& c21, int& c30, int& c31)
{
	Point pt00 = Point(x_c - x, y_c - y);
	--c00;
	int index00 = c00 + 100000;
	Point pt01 = Point(x_c + x, y_c - y);
	++c01;
	int index01 = c01 + 100000;

	Point pt10 = Point(x_c + y, y_c - x);
	--c10;
	int index10 = c10 + 200000;
	Point pt11 = Point(x_c + y, y_c + x);
	++c11;
	int index11 = c11 + 200000;

	Point pt20 = Point(x_c - x, y_c + y);
	++c20;
	int index20 = c20 + 300000;
	Point pt21 = Point(x_c + x, y_c + y);
	--c21;
	int index21 = c21 + 300000;

	Point pt30 = Point(x_c - y, y_c - x);
	++c30;
	int index30 = c30 + 400000;
	Point pt31 = Point(x_c - y, y_c + x);
	--c31;
	int index31 = c31 + 400000;

	PointIndex pt_index00 = PointIndex(pt00, index00);
	PointIndex pt_index01 = PointIndex(pt01, index01);

	PointIndex pt_index10 = PointIndex(pt10, index10);
	PointIndex pt_index11 = PointIndex(pt11, index11);

	PointIndex pt_index20 = PointIndex(pt20, index20);
	PointIndex pt_index21 = PointIndex(pt21, index21);

	PointIndex pt_index30 = PointIndex(pt30, index30);
	PointIndex pt_index31 = PointIndex(pt31, index31);

	if(std::find(result_out.begin(), result_out.end(), pt_index00) == result_out.end())
		result_out.push_back(pt_index00);

	if(std::find(result_out.begin(), result_out.end(), pt_index01) == result_out.end())
		result_out.push_back(pt_index01);

	if(std::find(result_out.begin(), result_out.end(), pt_index10) == result_out.end())
		result_out.push_back(pt_index10);

	if(std::find(result_out.begin(), result_out.end(), pt_index11) == result_out.end())
		result_out.push_back(pt_index11);

	if(std::find(result_out.begin(), result_out.end(), pt_index20) == result_out.end())
		result_out.push_back(pt_index20);

	if(std::find(result_out.begin(), result_out.end(), pt_index21) == result_out.end())
		result_out.push_back(pt_index21);

	if(std::find(result_out.begin(), result_out.end(), pt_index30) == result_out.end())
		result_out.push_back(pt_index30);

	if(std::find(result_out.begin(), result_out.end(), pt_index31) == result_out.end())
		result_out.push_back(pt_index31);
}

void midpoint_circle(int x_in, int y_in, int radius_in, vector<Point>& result_out)
{
	vector<PointIndex> point_index_vec;

	int c00 = 0;
	int c01 = 0;
	int c10 = 0;
	int c11 = 0;
	int c20 = 0;
	int c21 = 0;
	int c30 = 0;
	int c31 = 0;

	int p = 1 - radius_in;
    int x = 0;
    int y = radius_in;
    midpoint_circle_push_pixel(x, y, x_in, y_in, point_index_vec, c00, c01, c10, c11, c20, c21, c30, c31);

    while (x <= y)
    {
        ++x;
        if (p < 0)
            p += 2 * x;
        else
        {
            p += 2 * (x - y);
            --y;
        }
        midpoint_circle_push_pixel(x, y, x_in, y_in, point_index_vec, c00, c01, c10, c11, c20, c21, c30, c31);
    }

    sort(point_index_vec.begin(), point_index_vec.end(), compare_point_index_index());
    for (PointIndex& pt_index : point_index_vec)
    	result_out.push_back(pt_index.pt);
}

void bresenham_line(int x1_in, int y1_in, int const x2_in, int const y2_in, vector<Point>& result_out, const uchar count_threshold)
{
    int delta_x(x2_in - x1_in);
    signed char const ix((delta_x > 0) - (delta_x < 0));
    delta_x = abs(delta_x) << 1;
 
    int delta_y(y2_in - y1_in);
    signed char const iy((delta_y > 0) - (delta_y < 0));
    delta_y = abs(delta_y) << 1;
 
    result_out.push_back(Point(x1_in, y1_in));
 
    if (delta_x >= delta_y)
    {
        int error(delta_y - (delta_x >> 1));
 
        while (x1_in != x2_in)
        {
            if ((error >= 0) && (error || (ix > 0)))
            {
                error -= delta_x;
                y1_in += iy;
            }

            error += delta_y;
            x1_in += ix;
 			
 			result_out.push_back(Point(x1_in, y1_in));
    		if (result_out.size() == count_threshold || x1_in == 0 || y1_in == 0 || x1_in == WIDTH_SMALL_MINUS || y1_in == HEIGHT_SMALL_MINUS)
    			return;
        }
    }
    else
    {
        int error(delta_x - (delta_y >> 1));
 
        while (y1_in != y2_in)
        {
            if ((error >= 0) && (error || (iy > 0)))
            {
                error -= delta_y;
                x1_in += ix;
            }
            error += delta_x;
            y1_in += iy;
 
 			result_out.push_back(Point(x1_in, y1_in));
    		if (result_out.size() == count_threshold || x1_in == 0 || y1_in == 0 || x1_in == WIDTH_SMALL_MINUS || y1_in == HEIGHT_SMALL_MINUS)
    			return;
        }
    }
}

Point get_y_min_point(vector<Point>& pt_vec)
{
	Point pt_y_min = Point(0, 9999);
	for (Point& pt : pt_vec)
		if (pt.y < pt_y_min.y)
			pt_y_min = pt;

	return pt_y_min;
}

Point get_y_max_point(vector<Point>& pt_vec)
{
	Point pt_y_max = Point(0, -1);
	for (Point& pt : pt_vec)
		if (pt.y > pt_y_max.y)
			pt_y_max = pt;

	return pt_y_max;
}