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

#include "hand_splitter_new.h"
#include "mat_functions.h"

bool HandSplitterNew::compute(ForegroundExtractorNew& foreground_extractor, MotionProcessorNew& motion_processor, const string name)
{
	Mat image_visualization = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);

	Mat image_find_contours = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
	for (BlobNew& blob : *foreground_extractor.blob_detector.blobs)
		if (blob.active)
		{
			blob.fill(image_visualization, 254);
			blob.fill(image_find_contours, 254);
		}

	vector<vector<Point>> contours = legacyFindContours(image_find_contours);
	if (contours.size() == 0)
		return false;

	int complexity = 0;
	for (vector<Point>& contour : contours)
	{
		vector<Point> contour_approximated;
		approxPolyDP(Mat(contour), contour_approximated, 10, false);

		complexity += contour_approximated.size();

		Point pt_old = Point(-1, -1);
		for (Point& pt : contour_approximated)
		{
			if (pt_old.x != -1)
				line(image_visualization, pt_old, pt, Scalar(127), 2);
			
			pt_old = pt;
		}
	}

	bool b0 = complexity >= 15;

	imshow("image_visualizationadfasdfasdf", image_visualization);

	// int x_separator_middle = motion_processor.x_separator_middle;

	// primary_hand_blobs = vector<BlobNew>();
	// x_min_result = 9999;
	// x_max_result = 0;
	// y_min_result = 9999;
	// y_max_result = 0;

	// for (BlobNew& blob : *foreground_extractor.blob_detector.blobs)
	// 	if (blob.active)
	// 		if (blob.x > x_separator_middle)
	// 		{
	// 			primary_hand_blobs.push_back(blob);

	// 			if (blob.x_min < x_min_result)
	// 				x_min_result = blob.x_min;
	// 			if (blob.x_max > x_max_result)
	// 				x_max_result = blob.x_max;
	// 			if (blob.y_min < y_min_result)
	// 				y_min_result = blob.y_min;
	// 			if (blob.y_max > y_max_result)
	// 				y_max_result = blob.y_max;
	// 		}

	if (primary_hand_blobs.size() > 0)
		return true;
	else
		return false;
}