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

bool StereoProcessor::compute(MonoProcessorNew& mono_processor0, MonoProcessorNew& mono_processor1)
{
	vector<Point>* vec0 = &mono_processor0.points_unwrapped_result;
	vector<Point>* vec1 = &mono_processor1.points_unwrapped_result;
	Mat cost_mat = compute_cost_mat(*vec0, *vec1);
	vector<Point> indexes = compute_dtw_indexes(cost_mat);

	Mat image_visualization = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);

	for (Point& pt : indexes)
	{
		Point pt0 = (*vec0)[pt.x];
		Point pt1 = (*vec1)[pt.y];

		int x_diff = abs(pt0.x - pt1.x) * 10;
		if (x_diff > 254)
			x_diff = 254;

		image_visualization.ptr<uchar>(pt0.y, pt0.x)[0] = x_diff;
	}
	
	return false;
}