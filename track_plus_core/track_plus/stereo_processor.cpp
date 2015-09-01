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
	Mat cost_mat = compute_cost_mat(*vec0, *vec1, true);
	vector<Point> indexes = compute_dtw_indexes(cost_mat);

	Mat image_visualization = Mat::zeros(HEIGHT_LARGE, WIDTH_LARGE, CV_8UC1);

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
			line(image_visualization, pt1_large, pt0_large, Scalar(127), 1);
			circle(image_visualization, pt0_large, 5, Scalar(255), -1);
			circle(image_visualization, pt1_large, 5, Scalar(127), -1);

			// Point3f pt_reprojected = reprojector.reproject_to_3d(pt0_large.x, pt0_large.y, pt1_large.x, pt1_large.y);
			// Point pt_reprojected_2d = Point(pt_reprojected.x + 320, pt_reprojected.y + 240);
			// circle(image_visualization, pt_reprojected_2d, 5, Scalar(255), 1);
		}
	}

	imshow("image_visualizationd_fadfasdfdf", image_visualization);
	
	return false;
}