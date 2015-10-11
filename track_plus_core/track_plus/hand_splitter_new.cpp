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

void set_value(float* val_old, int val_new, int val_min, int val_max)
{
	if (val_new < val_min)
		val_new = val_min;
	if (val_new > val_max)
		val_new = val_max;

	*val_old = val_new;
}

bool HandSplitterNew::compute(ForegroundExtractorNew& foreground_extractor, MotionProcessorNew& motion_processor, string name, bool visualize)
{
	if (value_store.get_bool("first_pass", false) == false)
	{
		value_store.set_bool("first_pass", true);
		algo_name += name;
	}

	bool algo_name_found = false;
	for (String& algo_name_current : algo_name_vec_old)
		if (algo_name_current == algo_name)
		{
			algo_name_found = true;
			break;
		}

	LowPassFilter* low_pass_filter = value_store.get_low_pass_filter("low_pass_filter");

	//------------------------------------------------------------------------------------------------------------------------

	Mat image_find_contours = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
	for (BlobNew& blob : *foreground_extractor.blob_detector.blobs)
		if (blob.active)
			blob.fill(image_find_contours, 254);

	vector<vector<Point>> contours = legacyFindContours(image_find_contours);
	if (contours.size() == 0)
		return false;

	int complexity = 0;
	for (vector<Point>& contour : contours)
	{
		if (contour.size() <= 1)
			continue;

		vector<Point> contour_approximated;
		approxPolyDP(Mat(contour), contour_approximated, 10, false);
		complexity += contour_approximated.size();
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
			float dist0 = get_distance(pt, seed0, false);
			float dist1 = get_distance(pt, seed1, false);

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

	bool dual = false;
	if (seed_vec0.size() > 0 && seed_vec1.size() > 0)
	{
		x_seed_vec0_max = seed_vec0[seed_vec0.size() - 1].x;
		x_seed_vec1_min = seed_vec1[0].x;

		low_pass_filter->compute_if_smaller(x_seed_vec0_max, 0.5, "x_seed_vec0_max");
		low_pass_filter->compute_if_larger(x_seed_vec1_min, 0.5, "x_seed_vec1_min");

		value_store.set_float("x_seed_vec0_max", x_seed_vec0_max);
		value_store.set_float("x_seed_vec1_min", x_seed_vec1_min);

		value_store.set_bool("x_min_max_set", true);

		//------------------------------------------------------------------------------------------------------------------------

		int width0 = x_seed_vec0_max - x_min;
		int width1 = x_max - x_seed_vec1_min;
		int width_min = min(width0, width1);

		float gap_size = x_seed_vec1_min - x_seed_vec0_max;
		low_pass_filter->compute_if_larger(gap_size, 0.1, "gap_size");

		//------------------------------------------------------------------------------------------------------------------------

		bool bool_complexity = complexity >= 15;
		bool bool_width_min = width_min >= 20;
		bool bool_gap_size = gap_size >= 5;
		bool bool_gap_order = x_seed_vec0_max < x_seed_vec1_min;

		dual = bool_gap_order && bool_width_min && (bool_gap_size || bool_complexity);
	}

	//------------------------------------------------------------------------------------------------------------------------

	BlobNew* blob_max_size_left = NULL;
	BlobNew* blob_max_size_right = NULL;
	BlobNew* blob_max_size = NULL;

	for (BlobNew& blob : *foreground_extractor.blob_detector.blobs)
	{
		if (blob.x <= motion_processor.x_separator_middle && (blob_max_size_left == NULL || blob.count > blob_max_size_left->count))
			blob_max_size_left = &blob;
		else if (blob.x > motion_processor.x_separator_middle && (blob_max_size_right == NULL || blob.count > blob_max_size_right->count))
			blob_max_size_right = &blob;

		if (blob_max_size == NULL || blob.count > blob_max_size->count)
			blob_max_size = &blob;
	}

	//------------------------------------------------------------------------------------------------------------------------

	int count_left = 0;
	int count_right = 0;

	int x_min_left = 9999;
	int x_max_left = 0;
	int x_min_right = 9999;
	int x_max_right = 0;

	for (BlobNew& blob : *foreground_extractor.blob_detector.blobs)
		if (blob.active)
		{
			for (Point& pt : blob.data)
				if (pt.x < motion_processor.x_separator_middle)
				{
					++count_left;
					if (pt.x < x_min_left)
						x_min_left = pt.x;
					if (pt.x > x_max_left)
						x_max_left = pt.x;
				}
				else
				{
					++count_right;
					if (pt.x < x_min_right)
						x_min_right = pt.x;
					if (pt.x > x_max_right)
						x_max_right = pt.x;
				}
		}

	int width_left = x_max_left - x_min_left;
	int width_right = x_max_right - x_min_right;

	//------------------------------------------------------------------------------------------------------------------------

	bool reference_is_left = value_store.get_bool("reference_is_left", false);
	bool dual_old = value_store.get_bool("dual_old", dual);
	bool merge = value_store.get_bool("merge", false);

	if (!dual && dual_old)
	{
		float count_small = reference_is_left ? count_right : count_left;
		float count_large = reference_is_left ? count_left : count_right;

		if (count_small / count_large > 0.5)
		{
			merge = true;
			value_store.set_bool("merge", merge);
		}
	}

	dual_old = dual;
	value_store.set_bool("dual_old", dual_old);

	//------------------------------------------------------------------------------------------------------------------------

	int reference_x_offset = value_store.get_int("reference_x_offset", 0);
	int reference_x_offset_blob = value_store.get_int("reference_x_offset_blob", 0);

	bool do_reset = value_store.get_bool("do_reset", false);

	if (dual || !algo_name_found || do_reset)
	{
		if (dual)
		{
			motion_processor.compute_x_separator_middle = false;
			set_value(&motion_processor.x_separator_middle, (x_seed_vec1_min + x_seed_vec0_max) / 2, 0, WIDTH_SMALL_MINUS);
		}

		reference_is_left = count_left > count_right;
		value_store.set_bool("reference_is_left", reference_is_left);

		if (!algo_name_found || do_reset)
			set_value(&motion_processor.x_separator_middle, motion_processor.x_separator_middle_median, 0, WIDTH_SMALL_MINUS);

		int reference_x = reference_is_left ? foreground_extractor.x_min_result : foreground_extractor.x_max_result;
		reference_x_offset = reference_x - motion_processor.x_separator_middle;
		value_store.set_int("reference_x_offset", reference_x_offset);

		int reference_x_blob = -1;
		if (reference_is_left && blob_max_size_left != NULL)
			reference_x_blob = blob_max_size_left->x_max;
		else if (!reference_is_left && blob_max_size_right != NULL)
			reference_x_blob = blob_max_size_right->x_min;

		if (reference_x_blob != -1)
		{
			reference_x_offset_blob = reference_x_blob - motion_processor.x_separator_middle;
			value_store.set_int("reference_x_offset_blob", reference_x_offset_blob);
		}

		merge = false;
		value_store.set_bool("merge", merge);

		do_reset = false;
		value_store.set_bool("do_reset", do_reset);
	}

	//------------------------------------------------------------------------------------------------------------------------

	if (!dual && !merge)
	{
		int reference_x = reference_is_left ? foreground_extractor.x_min_result : foreground_extractor.x_max_result;
		int reference_x_blob = reference_is_left ? blob_max_size->x_max : blob_max_size->x_min;

		const int x_separator_middle_target = reference_x_blob - reference_x_offset_blob;
		const int x_separator_middle = motion_processor.x_separator_middle;
		const int i_increment = x_separator_middle_target > motion_processor.x_separator_middle ? 1 : -1;

		int cost = 0;
		for (int i = x_separator_middle; i != x_separator_middle_target; i += i_increment)
			for (int j = 0; j < HEIGHT_SMALL; ++j)
				if (foreground_extractor.image_foreground.ptr<uchar>(j, i)[0] > 0)
					++cost;

		if ((float)cost / blob_max_size->count < 0.5)
			set_value(&motion_processor.x_separator_middle, x_separator_middle_target, 0, WIDTH_SMALL_MINUS);
		else
		{
			do_reset = true;
			value_store.set_bool("do_reset", do_reset);
		}
	}

	//------------------------------------------------------------------------------------------------------------------------

	Point seed_left = value_store.get_point("seed_left", Point(x_min, 0));
	Point seed_right = value_store.get_point("seed_right", Point(x_max, 0));

	if (merge || dual)
	{
		seed_left = Point(0, 0);
		int seed_left_count = 0;
		for (BlobNew& blob : blobs_left)
			for (Point& pt : blob.data)
			{
				seed_left.x += pt.x;
				seed_left.y += pt.y;
				++seed_left_count;
			}
		
		seed_right = Point(0, 0);
		int seed_right_count = 0;
		for (BlobNew& blob : blobs_right)
			for (Point& pt : blob.data)
			{
				seed_right.x += pt.x;
				seed_right.y += pt.y;
				++seed_right_count;
			}

		if (seed_left_count > 0 && seed_right_count > 0)
		{
			seed_left.x /= seed_left_count;
			seed_left.y /= seed_left_count;

			seed_right.x /= seed_right_count;
			seed_right.y /= seed_right_count;

			vector<Point> seed_left_vec;
			vector<Point> seed_right_vec;

			while (true)
			{
				seed_left_vec.clear();
				seed_right_vec.clear();				

				for (BlobNew& blob : *foreground_extractor.blob_detector.blobs)
					if (blob.active)
						for (Point& pt : blob.data)
						{
							float dist_left = get_distance(pt, seed_left, false);
							float dist_right = get_distance(pt, seed_right, false);
							if (dist_left < dist_right)
								seed_left_vec.push_back(pt);
							else
								seed_right_vec.push_back(pt);
						}

				if (seed_left_vec.size() > 0 && seed_right_vec.size() > 0)
				{
					Point seed_left_new = Point(0, 0);
					for (Point& pt : seed_left_vec)
					{
						seed_left_new.x += pt.x;
						seed_left_new.y += pt.y;
					}
					seed_left_new.x /= seed_left_vec.size();
					seed_left_new.y /= seed_left_vec.size();

					Point seed_right_new = Point(0, 0);
					for (Point& pt : seed_right_vec)
					{
						seed_right_new.x += pt.x;
						seed_right_new.y += pt.y;
					}
					seed_right_new.x /= seed_right_vec.size();
					seed_right_new.y /= seed_right_vec.size();

					if (seed_left.x == seed_left_new.x && seed_left.y == seed_left_new.y &&
						seed_right.x == seed_right_new.x && seed_right.y == seed_right_new.y)
					{
						break;
					}

					seed_left = seed_left_new;
					seed_right = seed_right_new;
				}
				else
					break;
			}

			if (merge)
				set_value(&motion_processor.x_separator_middle, (seed_left.x + seed_right.x) / 2, 0, WIDTH_SMALL_MINUS);
		}
	}

	value_store.set_point("seed_left", seed_left);
	value_store.set_point("seed_right", seed_right);

	//------------------------------------------------------------------------------------------------------------------------

	const int x_separator_middle = motion_processor.x_separator_middle;
	const int x_separator_middle_median = motion_processor.x_separator_middle_median;
	const int x_separator_left_median = motion_processor.x_separator_left_median;
	const int x_separator_right_median = motion_processor.x_separator_right_median;
	const int y_separator_down_median = motion_processor.y_separator_down_median;
	const int y_separator_up_median = motion_processor.y_separator_up_median;

	while (dual)
	{
		const int i_min = seed_left.x;
		const int i_max = seed_right.x;
		const int j_min = 0;
		const int j_max = y_separator_up_median;

		if (!(i_max > i_min && i_min >= 0 && i_max <= WIDTH_SMALL_MINUS))
			break;

		Point pt0 = Point(i_min, 0);
		Point pt1 = Point(i_max, 0);
		Point pt2 = Point(x_separator_middle, j_max);    //tip point

		if (pt2.y < 2 || i_max - i_min < 2)
			break;

		Mat image_triangle_fill = Mat::zeros(j_max + 1, WIDTH_SMALL, CV_8UC1);

		line(image_triangle_fill, pt0, pt2, Scalar(254), 1);
		line(image_triangle_fill, pt1, pt2, Scalar(254), 1);
		floodFill(image_triangle_fill, Point(x_separator_middle, 0), Scalar(254));

		for (int i = i_min; i <= i_max; ++i)
			for (int j = j_min; j <= j_max; ++j)
				if (image_triangle_fill.ptr<uchar>(j, i)[0] > 0)
					motion_processor.fill_image_background_static(i, j, motion_processor.image_ptr);

		break;
	}

	//------------------------------------------------------------------------------------------------------------------------

	x_min_result_right = 9999;
	x_max_result_right = -1;
	y_min_result_right = 9999;
	y_max_result_right = -1;

	x_min_result_left = 9999;
	x_max_result_left = -1;
	y_min_result_left = 9999;
	y_max_result_left = -1;

	blobs_right.clear();
	blobs_left.clear();

	for (BlobNew& blob : *foreground_extractor.blob_detector.blobs)
		if (blob.active)
		{
			if (blob.x > motion_processor.x_separator_middle)
			{
				if (blob.x_min < x_min_result_right)
					x_min_result_right = blob.x_min;
				if (blob.x_max > x_max_result_right)
					x_max_result_right = blob.x_max;
				if (blob.y_min < y_min_result_right)
					y_min_result_right = blob.y_min;
				if (blob.y_max > y_max_result_right)
					y_max_result_right = blob.y_max;

				blobs_right.push_back(blob);
			}
			else
			{
				if (blob.x_min < x_min_result_left)
					x_min_result_left = blob.x_min;
				if (blob.x_max > x_max_result_left)
					x_max_result_left = blob.x_max;
				if (blob.y_min < y_min_result_left)
					y_min_result_left = blob.y_min;
				if (blob.y_max > y_max_result_left)
					y_max_result_left = blob.y_max;

				blobs_left.push_back(blob);
			}
		}

	//------------------------------------------------------------------------------------------------------------------------

	if (visualize)
	{
		Mat image_visualization = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
		for (BlobNew& blob : *foreground_extractor.blob_detector.blobs)
			if (blob.active)
				blob.fill(image_visualization, 254);

		line(image_visualization, Point(x_separator_left_median, 0), Point(x_separator_left_median, 999), Scalar(254), 1);
		line(image_visualization, Point(x_separator_right_median, 0), Point(x_separator_right_median, 999), Scalar(254), 1);
		line(image_visualization, Point(x_separator_middle, 0), Point(x_separator_middle, 999), Scalar(254), 1);
		line(image_visualization, Point(0, y_separator_down_median), Point(999, y_separator_down_median), Scalar(254), 1);
		line(image_visualization, Point(0, y_separator_up_median), Point(999, y_separator_up_median), Scalar(254), 1);

		imshow("image_visualizationsdlkfjhasdf" + name, image_visualization);
	}

	if (blobs_right.size() > 0 || blobs_left.size() > 0)
	{
		algo_name_vec.push_back(algo_name);
		return true;
	}

	return false;
}