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

#include "stereo_processor.h"

bool StereoProcessor::compute(MonoProcessorNew& mono_processor0, MonoProcessorNew& mono_processor1,
							  Reprojector& reprojector,          PointerMapper& pointer_mapper)
{
	vector<Point>* vec0 = &mono_processor0.stereo_matching_points;
	vector<Point>* vec1 = &mono_processor1.stereo_matching_points;
	Mat cost_mat = compute_cost_mat(*vec0, *vec1);
	vector<Point> indexes = compute_dtw_indexes(cost_mat);

	Mat image_visualization = Mat::zeros(HEIGHT_LARGE, WIDTH_LARGE, CV_8UC1);

	Point pt0_large_old = Point(-1, -1);
	Point pt1_large_old = Point(-1, -1);
	for (Point& pt : indexes)
	{
		Point pt0 = (*vec0)[pt.x];
		Point pt1 = (*vec1)[pt.y];

		Point pt0_large = pt0 * 4;
		Point pt1_large = pt1 * 4;

		if (pointer_mapper.calibrated)
		{

		}
		else
		{
			// if (pt0_large_old.x != -1)
				// line(image_visualization, pt0_large, pt0_large_old, Scalar(255), 1);
			// if (pt1_large_old.x != -1)
				// line(image_visualization, pt1_large, pt1_large_old, Scalar(127), 1);

			line(image_visualization, pt1_large, pt0_large, Scalar(127), 1);
			circle(image_visualization, pt0_large, 5, Scalar(255), -1);
			circle(image_visualization, pt1_large, 5, Scalar(127), -1);

			// Point3f pt_reprojected = reprojector.reproject_to_3d(pt0.x, pt0.y, pt1.x, pt1.y);
			// Point pt_reprojected_2d = Point(pt_reprojected.x / 50 + 320, pt_reprojected.y / 40 + 240);
			// circle(image_visualization, pt_reprojected_2d, 5, Scalar(255), 1);
		}

		pt0_large_old = pt0_large;
		pt1_large_old = pt1_large;
	}

	imshow("image_visualizationd_fadfasdfdf", image_visualization);
	
	return false;
}