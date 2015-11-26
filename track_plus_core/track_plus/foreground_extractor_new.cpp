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

#include "foreground_extractor_new.h"
#include "mat_functions.h"

struct compare_point_x
{
	inline bool operator() (const Point pt0_in, const Point pt1_in)
	{
		return pt0_in.x < pt1_in.x;
	}
};

bool ForegroundExtractorNew::compute(Mat& image_in, MotionProcessorNew& motion_processor, const string name, const bool visualize)
{
	if (value_store.get_bool("first_pass", false) == false)
	{
		value_store.set_bool("first_pass", true);
		algo_name += name;
	}

	//------------------------------------------------------------------------------------------------------------------------

	const int x_separator_middle = motion_processor.x_separator_middle;
	const int x_separator_left = motion_processor.x_separator_left;
	const int x_separator_right = motion_processor.x_separator_right;
	const int y_separator_down = motion_processor.y_separator_down;
	const int y_separator_up = motion_processor.y_separator_up;

	const int x_separator_middle_median = motion_processor.x_separator_middle_median;
	const int x_separator_left_median = motion_processor.x_separator_left_median;
	const int x_separator_right_median = motion_processor.x_separator_right_median;
	const int y_separator_down_median = motion_processor.y_separator_down_median;
	const int y_separator_up_median = motion_processor.y_separator_up_median;

	const int gray_threshold_left = motion_processor.gray_threshold_left;
	const int gray_threshold_right = motion_processor.gray_threshold_right;

	const int diff_threshold = motion_processor.diff_threshold;
	Mat image_background_static = motion_processor.image_background_static;

	//------------------------------------------------------------------------------------------------------------------------

	image_foreground = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);

	for (int i = 0; i < WIDTH_SMALL; ++i)
		for (int j = 0; j < HEIGHT_SMALL; ++j)
		{
			const uchar gray_current = image_in.ptr<uchar>(j, i)[0];

			if ((i <= x_separator_middle && gray_current > gray_threshold_left) ||
				(i > x_separator_middle && gray_current > gray_threshold_right))
			{
				if (image_background_static.ptr<uchar>(j, i)[0] == 255)
					image_foreground.ptr<uchar>(j, i)[0] = 254;
				else
					image_foreground.ptr<uchar>(j, i)[0] = abs(gray_current - image_background_static.ptr<uchar>(j, i)[0]);
			}
		}

	threshold(image_foreground, image_foreground, diff_threshold, 254, THRESH_BINARY);
	// GaussianBlur(image_foreground, image_foreground, Size(5, 5), 0, 0);
	// threshold(image_foreground, image_foreground, 150, 254, THRESH_BINARY);

	//------------------------------------------------------------------------------------------------------------------------

	blob_detector.compute(image_foreground, 254, 0, WIDTH_SMALL, 0, HEIGHT_SMALL, false);

	int x_min_result_temp = 9999;
	int x_max_result_temp = -1;
	int y_min_result_temp = 9999;
	int y_max_result_temp = -1;
	int count_result_temp = -1;

	int active_count = 0;
	for (BlobNew& blob : *blob_detector.blobs)
		if (blob.y > y_separator_down || blob.y_min > y_separator_down_median || /*blob.y_max < y_separator_up_median ||*/
			blob.x_max < x_separator_left || blob.x_max < x_separator_left_median ||
			blob.x_min > x_separator_right || blob.x_min > x_separator_right_median ||
			blob.count < blob_detector.blob_max_size->count / 20)
		{
			blob.active = false;
			blob.fill(image_foreground, 0);
		}
		else
		{
			if (blob.x_min < x_min_result_temp)
				x_min_result_temp = blob.x_min;
			if (blob.x_max > x_max_result_temp)
				x_max_result_temp = blob.x_max;
			if (blob.y_min < y_min_result_temp)
				y_min_result_temp = blob.y_min;
			if (blob.y_max > y_max_result_temp)
				y_max_result_temp = blob.y_max;

			count_result_temp += blob.count;
			++active_count;
		}

	if (active_count == 0)
		return false;

	if (x_min_result_temp < 9999 && x_max_result_temp > -1 && y_min_result_temp < 9999 && y_max_result_temp > -1)
	{
		x_min_result = x_min_result_temp;
		x_max_result = x_max_result_temp;
		y_min_result = y_min_result_temp;
		y_max_result = y_max_result_temp;
		count_result = count_result_temp;
	}

	//------------------------------------------------------------------------------------------------------------------------

	if (motion_processor.will_compute_next_frame /*&& !value_store.get_bool("pass", false)*/)
	{
		// value_store.set_bool("pass", true);

		Mat image_foreground_processed;
		GaussianBlur(image_foreground, image_foreground_processed, Size(1, 29), 0, 0);
		threshold(image_foreground_processed, image_foreground_processed, 1, 254, THRESH_BINARY);
		erode(image_foreground_processed, image_foreground_processed, Mat(), Point(-1, -1), 5);

		const int j_max = (y_separator_down - y_separator_up) / 5 + y_separator_up;
		for (int i = 0; i < WIDTH_SMALL; ++i)
			for (int j = j_max; j >= 0; --j)
				if (image_foreground_processed.ptr<uchar>(j, i)[0] > 0)
					if (motion_processor.image_borders_public.ptr<uchar>(j, i)[0] == 0)
						image_background_static.ptr<uchar>(j, i)[0] = 255;
	}

	//------------------------------------------------------------------------------------------------------------------------

	if (visualize && enable_imshow)
	{
		Mat image_visualization = image_foreground.clone();
		line(image_visualization, Point(x_separator_left_median, 0), Point(x_separator_left_median, 999), Scalar(254), 1);
		line(image_visualization, Point(x_separator_right_median, 0), Point(x_separator_right_median, 999), Scalar(254), 1);
		line(image_visualization, Point(x_separator_middle_median, 0), Point(x_separator_middle_median, 999), Scalar(254), 1);
		line(image_visualization, Point(0, y_separator_down_median), Point(999, y_separator_down_median), Scalar(254), 1);
		line(image_visualization, Point(0, y_separator_up_median), Point(999, y_separator_up_median), Scalar(254), 1);
		// line(image_visualization, Point(0, y_ref), Point(999, y_ref), Scalar(254), 1);

		imshow("image_visualizationadfasdfdff" + name, image_visualization);
	}

	algo_name_vec.push_back(algo_name);
	return true;
}