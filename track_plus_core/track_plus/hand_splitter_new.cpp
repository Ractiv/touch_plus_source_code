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
	LowPassFilter* low_pass_filter = value_store.get_low_pass_filter("low_pass_filter");

	//------------------------------------------------------------------------------------------------------------------------

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

	//------------------------------------------------------------------------------------------------------------------------

	int x_min = foreground_extractor.x_min_result;
	int x_max = foreground_extractor.x_max_result;
	int y_min = foreground_extractor.y_min_result;
	int y_max = foreground_extractor.y_max_result;

	float x_seed_vec0_max = value_store.get_float("x_seed_vec0_max");
	float x_seed_vec1_min = value_store.get_float("x_seed_vec1_min");

	int intensity_array[WIDTH_SMALL] { 0 };
	for (BlobNew& blob : *foreground_extractor.blob_detector.blobs)
		if (blob.active)
			for (Point& pt : blob.data)
				++intensity_array[pt.x];

	vector<Point> hist_pt_vec;
	for (int i = 0; i < WIDTH_SMALL; ++i)
	{
		int j = intensity_array[i];
		low_pass_filter->compute(j, 0.5, "histogram_j");

		if (j < 10)
			continue;

		for (int j_current = 0; j_current < j; ++j_current)
			hist_pt_vec.push_back(Point(i, j_current));
	}

	Point seed0 = Point(x_min, 0);
	Point seed1 = Point(x_max, 0);

	vector<Point> seed_vec0;
	vector<Point> seed_vec1;

	while (true)
	{
		seed_vec0.clear();
		seed_vec1.clear();

		for (Point& pt : hist_pt_vec)
		{
			float dist0 = get_distance(pt, seed0);
			float dist1 = get_distance(pt, seed1);

			if (dist0 < dist1)
				seed_vec0.push_back(pt);
			else
				seed_vec1.push_back(pt);
		}

		if (seed_vec0.size() == 0 || seed_vec1.size() == 0)
			break;

		Point seed0_new = Point(0, 0);
		for (Point& pt : seed_vec0)
		{
			seed0_new.x += pt.x;
			seed0_new.y += pt.y;
		}
		seed0_new.x /= seed_vec0.size();
		seed0_new.y /= seed_vec0.size();

		Point seed1_new = Point(0, 0);
		for (Point& pt : seed_vec1)
		{
			seed1_new.x += pt.x;
			seed1_new.y += pt.y;
		}
		seed1_new.x /= seed_vec1.size();
		seed1_new.y /= seed_vec1.size();

		if (seed0 == seed0_new && seed1 == seed1_new)
			break;

		seed0 = seed0_new;
		seed1 = seed1_new;
	}

	if (seed_vec0.size() > 0 && seed_vec1.size() > 0)
	{
		x_seed_vec0_max = seed_vec0[seed_vec0.size() - 1].x;
		x_seed_vec1_min = seed_vec1[0].x;

		low_pass_filter->compute_if_smaller(x_seed_vec0_max, 0.5, "x_seed_vec0_max");
		low_pass_filter->compute_if_larger(x_seed_vec1_min, 0.5, "x_seed_vec1_min");

		value_store.set_float("x_seed_vec0_max", x_seed_vec0_max);
		value_store.set_float("x_seed_vec1_min", x_seed_vec1_min);

		value_store.set_bool("x_min_max_set", true);

		#if 1
		{
			Mat image_histogram = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);

			for (Point& pt : seed_vec0)
				line(image_histogram, pt, Point(pt.x, 0), Scalar(127), 1);

			for (Point& pt : seed_vec1)
				line(image_histogram, pt, Point(pt.x, 0), Scalar(254), 1);

			circle(image_histogram, seed0, 5, Scalar(64), -1);
			circle(image_histogram, seed1, 5, Scalar(64), -1);

			line(image_histogram, Point(x_seed_vec0_max, 0), Point(x_seed_vec0_max, 9999), Scalar(64), 1);
			line(image_histogram, Point(x_seed_vec1_min, 0), Point(x_seed_vec1_min, 9999), Scalar(64), 1);

			imshow("image_histogramasd" + name, image_histogram);
		}
		#endif
	}

	int width0 = x_max - x_seed_vec1_min;
	int width1 = x_seed_vec0_max - x_min;
	int width_min = min(width0, width1);

	float gap_size = x_seed_vec1_min - x_seed_vec0_max;
	low_pass_filter->compute_if_larger(gap_size, 0.1, "gap_size");

	//------------------------------------------------------------------------------------------------------------------------

	bool bool_complexity = complexity >= 15;
	bool bool_width_min = width_min >= 20;
	bool bool_gap_size = gap_size >= 5;

	bool dual_temp = false;
	if (bool_width_min && (bool_gap_size || bool_complexity))
		dual_temp = true;

	int width = x_max - x_min;
	int width_old = value_store.get_int("width_old", width);

	int x_separator_middle_temp = (x_seed_vec0_max + x_seed_vec1_min) / 2;
	int x_separator_middle_temp_old = value_store.get_int("x_separator_middle_temp_old", x_separator_middle_temp);

	bool dual_temp_old = value_store.get_bool("dual_temp_old", dual_temp);
	if (!dual_temp && dual_temp_old)
	{
		COUT << wdith0 + width1 << endl;
		// COUT << abs(width - width_old) << " " << (x_separator_middle_temp - x_separator_middle_temp_old) << endl;
	}

	value_store.set_bool("dual_temp_old", dual_temp);
	value_store.set_int("width_old", width);
	value_store.set_int("x_separator_middle_temp_old", x_separator_middle_temp);

	// int subject_width = value_store.get_int("subject_width", width0 + width1);
	// if (dual_temp)
	// 	value_store.set_int("subject_width", width0 + width1);
	// else
	// {
	// 	COUT << subject_width - (x_max - x_min) << endl;
	// }


	//------------------------------------------------------------------------------------------------------------------------

	// if (dual)
	// 	x_separator_middle = (x_seed_vec0_max + x_seed_vec1_min) / 2;

	// value_store.set_int("x_separator_middle", x_separator_middle);

	// line(image_visualization, Point(x_separator_middle, 0), Point(x_separator_middle, 999), Scalar(127), 1);

	// imshow("image_visualizationadfasdfasdf", image_visualization);

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