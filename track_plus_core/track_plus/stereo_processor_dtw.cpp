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

#include "stereo_processor_dtw.h"
#include "dtw.h"

struct PointPair
{
	Point pt0;
	Point pt1;
	int y_diff;

	PointPair(Point& _pt0, Point& _pt1, int _y_diff)
	{
		pt0 = _pt0;
		pt1 = _pt1;
		y_diff = _y_diff;
	}
};

void center_pt_vec(vector<Point>& contour_processed, vector<Point>& stereo_matching_points,
						    Point& pt_y_max, Point& pt_palm, int palm_radius)
{
	int x_diff = (WIDTH_SMALL / 2) - (pt_palm.x - palm_radius);
	int y_diff = (HEIGHT_SMALL / 2) - pt_y_max.y;

	for (Point& pt : contour_processed)
	{
		Point pt_new = Point(pt.x + x_diff, pt.y + y_diff);
		stereo_matching_points.push_back(pt_new);
	}
}

bool StereoProcessorDTW::compute(SCOPA& scopa0, SCOPA& scopa1, PointResolver& point_resolver,
							     PointerMapper& pointer_mapper,     Mat& image0,                       Mat& image1)
{
	vector<Point> vec0_raw = scopa0.stereo_matching_points;
	vector<Point> vec1_raw = scopa1.stereo_matching_points;

	Point y_max_point0 = get_y_max_point(vec0_raw);
	Point y_max_point1 = get_y_max_point(vec1_raw);

	vector<Point> vec0;
	center_pt_vec(vec0_raw, vec0, y_max_point0, scopa0.pt_palm, scopa0.palm_radius);

	vector<Point> vec1;
	center_pt_vec(vec1_raw, vec1, y_max_point1, scopa1.pt_palm, scopa1.palm_radius);

	Mat cost_mat = compute_cost_mat(vec0, vec1, true);
	vector<Point> indexes = compute_dtw_indexes(cost_mat);

	Mat image_visualization = Mat::zeros(HEIGHT_LARGE, WIDTH_LARGE, CV_8UC1);

	for (Point& index_pair : indexes)
	{
		Point pt0_raw = vec0_raw[index_pair.x];
		Point pt1_raw = vec1_raw[index_pair.y];

		Point pt0 = vec0[index_pair.x];
		Point pt1 = vec1[index_pair.y];

		// int y_diff = abs(pt0.y - pt1.y);
		// if (y_diff >= 3)
			// continue;

		// line(image_visualization, pt0_raw, pt1_raw, Scalar(254), 1);

		Point2f pt_resolved0 = point_resolver.compute(pt0_raw, image0, 0);
		Point2f pt_resolved1 = point_resolver.compute(pt1_raw, image1, 1);

		if (pt_resolved0.x == 9999 || pt_resolved1.x == 9999)
			continue;

		Point3f pt3d = point_resolver.reprojector->reproject_to_3d(pt_resolved0.x, pt_resolved0.y, pt_resolved1.x, pt_resolved1.y);
		// circle(image_visualization, Point(320 + pt3d.x, 240 + pt3d.y), pow(1000 / (pt3d.z + 1), 2), Scalar(254), 1);

		if (pt3d.z < 0)
			pt3d.z = 0;
		else if (pt3d.z > 255)
			pt3d.z = 255;

		image_visualization.ptr<uchar>(pt0_raw.y, pt0_raw.x)[0] = pt3d.z;
	}

	imshow("image_visualizationadsfasdfasdf", image_visualization);

	//------------------------------------------------------------------------------------------------------------------------------

	algo_name_vec.push_back(algo_name);
	return true;
}