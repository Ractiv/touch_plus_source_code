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

#include "motion_processor_new.h"

bool MotionProcessorNew::compute(Mat& image_in, const string name, const bool visualize)
{
	static bool large_dilation = true;
	static bool dilated0 = false;
	static bool dilated1 = false;

	value_store.set_bool("both_hands_are_moving", false);
	value_store.set_bool("left_hand_is_moving", false);
	value_store.set_bool("right_hand_is_moving", false);

	Mat image_background = value_store.get_mat("image_background");
	Mat image_foreground = value_store.get_mat("image_foreground", true);

	LowPassFilter* low_pass_filter = value_store.get_low_pass_filter("low_pass_filter");

	if (value_store.get_bool("image_background_set") &&
		value_store.get_int("current_frame") >= value_store.get_int("target_frame", 3))
	{
		value_store.set_int("current_frame", 0);

		uchar motion_diff_max = 0;
		Mat image_subtraction = Mat(image_in.size(), CV_8UC1);

		for (int i = 0; i < WIDTH_SMALL; ++i)
			for (int j = 0; j < HEIGHT_SMALL; ++j)
			{
				const uchar diff = abs(image_in.ptr<uchar>(j, i)[0] - image_background.ptr<uchar>(j, i)[0]);
				if (diff > motion_diff_max)
					motion_diff_max = diff;

				image_subtraction.ptr<uchar>(j, i)[0] = diff;
			}

		threshold(image_subtraction, image_subtraction, motion_diff_max * 0.25, 254, THRESH_BINARY);

		BlobDetectorNew* blob_detector_image_subtraction = value_store.get_blob_detector("blob_detector_image_subtraction");
		blob_detector_image_subtraction->compute(image_subtraction, 254, 0, WIDTH_SMALL, 0, HEIGHT_SMALL, false);

		if (blob_detector_image_subtraction->blobs->size() < 100)
		{
			for (BlobNew& blob : *(blob_detector_image_subtraction->blobs))
				if (blob.count < noise_size || (y_separator_motion_down_median > -1 && blob.y > y_separator_motion_down_median))
				{
					blob.active = false;
					blob.fill(image_subtraction, 0);
				}

			bool proceed0 = compute_x_separator_middle();
			bool proceed1 = compute_x_separator_motion_left_right();
			bool proceed2 = compute_y_separator_motion();

			if (proceed0 && proceed1 && proceed2)
				value_store.set_bool("separators_computed", true);

			if (value_store.get_bool("separators_computed"))
			{
				int left_y_max = -1;
				int right_y_max = -1;

				if (!value_store.get_bool("both_hands_are_moving"))
				{
					int left_count = 0;
					int right_count = 0;

					for (BlobNew& blob : *(blob_detector_image_subtraction->blobs))
						if (blob.active)
							if (blob.x < x_separator_middle_median)
								++left_count;
							else
								++right_count;

					if (left_count > right_count)
						value_store.set_bool("left_hand_is_moving", true);
					else
						value_store.set_bool("right_hand_is_moving", true);
				}
				else
				{
					value_store.set_bool("left_hand_is_moving", true);
					value_store.set_bool("right_hand_is_moving", true);

					for (BlobNew& blob : *(blob_detector_image_subtraction->blobs))
						if (blob.active)
							if (blob.x < x_separator_middle_median)
							{
								if (blob.y_max > left_y_max)
									left_y_max = blob.y_max;
							}
							else
							{
								if (blob.y_max > right_y_max)
									right_y_max = blob.y_max;
							}
				}

				static string motion_processor_primary_name = "";

				if (motion_processor_primary_name == "")
					motion_processor_primary_name = name;

				static float gray_threshold_stereo;
				if (name == motion_processor_primary_name)
				{
					vector<uchar> gray_vec;
					for (BlobNew& blob : *(blob_detector_image_subtraction->blobs))
						if (blob.active)
							for (Point pt : blob.data)
							{
								uchar gray = std::max(image_in.ptr<uchar>(pt.y, pt.x)[0], image_background.ptr<uchar>(pt.y, pt.x)[0]);
								gray_vec.push_back(gray);
							}

					if (gray_vec.size() > 0)
					{
						sort(gray_vec.begin(), gray_vec.end());
						float gray_max = gray_vec[gray_vec.size() * 0.9];
						float gray_median = gray_vec[gray_vec.size() * 0.5];

						gray_threshold = gray_median - 30;
						low_pass_filter->compute(gray_threshold, 0.1, "gray_threshold");

						gray_threshold_stereo = gray_threshold;
					}
				}
				else
					gray_threshold = gray_threshold_stereo;

				Mat image_in_thresholded;
				threshold(image_in, image_in_thresholded, gray_threshold, 254, THRESH_BINARY);

				if (value_store.get_int("compute_slope_count") <= 10)
				{
					compute_slope(image_in_thresholded, "left");
					compute_slope(image_in_thresholded, "right");

					vector<Point>* pt_top_left_vec = value_store.get_point_vec("pt_top_left_vec");
					vector<Point>* pt_bottom_left_vec = value_store.get_point_vec("pt_bottom_left_vec");
					vector<Point>* pt_top_right_vec = value_store.get_point_vec("pt_top_right_vec");
					vector<Point>* pt_bottom_right_vec = value_store.get_point_vec("pt_bottom_right_vec");

					{
						Point pt_top = value_store.get_point("pt_top_left", Point(9999, 0));
						Point pt_bottom = value_store.get_point("pt_bottom_left", Point(9999, 0));

						if (pt_top.x != 9999 && pt_bottom.x != 9999)
						{
							pt_top_left_vec->push_back(pt_top);
							pt_bottom_left_vec->push_back(pt_bottom);
							sort(pt_top_left_vec->begin(), pt_top_left_vec->end(), compare_point_x());
							sort(pt_bottom_left_vec->begin(), pt_bottom_left_vec->end(), compare_point_x());
						}
					}

					{
						Point pt_top = value_store.get_point("pt_top_right", Point(9999, 0));
						Point pt_bottom = value_store.get_point("pt_bottom_right", Point(9999, 0));

						if (pt_top.x != 9999 && pt_bottom.x != 9999)
						{
							pt_top_right_vec->push_back(pt_top);
							pt_bottom_right_vec->push_back(pt_bottom);
							sort(pt_top_right_vec->begin(), pt_top_right_vec->end(), compare_point_x());
							sort(pt_bottom_right_vec->begin(), pt_bottom_right_vec->end(), compare_point_x());
						}
					}

					Mat image_borders = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);

					if (pt_top_left_vec->size() >= 5)
					{
						Point pt_top = (*pt_top_left_vec)[pt_top_left_vec->size() / 2];
						Point pt_bottom = (*pt_bottom_left_vec)[pt_bottom_left_vec->size() / 2];
						line(image_borders, pt_top, pt_bottom, Scalar(254), 1);
						floodFill(image_borders, Point(0, 0), Scalar(254));
					}

					if (pt_top_right_vec->size() >= 5)
					{
						Point pt_top = (*pt_top_right_vec)[pt_top_right_vec->size() / 2];
						Point pt_bottom = (*pt_bottom_right_vec)[pt_bottom_right_vec->size() / 2];
						line(image_borders, pt_top, pt_bottom, Scalar(254), 1);
						floodFill(image_borders, Point(WIDTH_SMALL_MINUS, 0), Scalar(254));
					}

					for (int i = 0; i < WIDTH_SMALL; ++i)
						for (int j = 0; j < HEIGHT_SMALL; ++j)
							if (image_borders.ptr<uchar>(j, i)[0] > 0)
								fill_image_background_static(i, j, image_in);

					if (visualize && enable_imshow)
						imshow("image_borders" + name, image_borders);

					value_store.set_int("compute_slope_count", value_store.get_int("compute_slope_count") + 1);
				}

				if (value_store.get_bool("image_bottom_sides_filled") == false && y_separator_motion_down_median != -1)
				{					
					value_store.set_bool("image_bottom_sides_filled", true);

					int x_separator_motion_left_median_const = x_separator_motion_left_median;
					int x_separator_motion_right_median_const = x_separator_motion_right_median;

					for (int i = 0; i < WIDTH_SMALL; ++i)
						for (int j = y_separator_motion_down_median; j < HEIGHT_SMALL; ++j)
							fill_image_background_static(i, j, image_in);

					for (int i = 0; i <= x_separator_motion_left_median_const; ++i)
						for (int j = 0; j < HEIGHT_SMALL; ++j)
							fill_image_background_static(i, j, image_in);

					for (int i = x_separator_motion_right_median_const; i < WIDTH_SMALL; ++i)
						for (int j = 0; j < HEIGHT_SMALL; ++j)
							fill_image_background_static(i, j, image_in);	
				}

				Mat image_in_thresholded_dilated;

				if (large_dilation == true)
				{
					dilate(image_in_thresholded, image_in_thresholded_dilated, Mat(), Point(-1, -1), 20);
					dilated0 = true;
				}
				else
					dilate(image_in_thresholded, image_in_thresholded_dilated, Mat(), Point(-1, -1), 3);

				for (int i = 0; i < WIDTH_SMALL; ++i)
					for (int j = 0; j < HEIGHT_SMALL; ++j)
						if (image_in_thresholded_dilated.ptr<uchar>(j, i)[0] == 0)
							fill_image_background_static(i, j, image_in);

				Mat image_foreground_dilated;

				if (large_dilation == true)
				{
					dilate(image_foreground, image_foreground_dilated, Mat(), Point(-1, -1), 20);
					dilated1 = true;
				}
				else
					dilate(image_foreground, image_foreground_dilated, Mat(), Point(-1, -1), 3);

				bool fill_image_foreground_dilated = false;
				for (int i = 0; i < WIDTH_SMALL; ++i)
					for (int j = 0; j < HEIGHT_SMALL; ++j)
						if (image_foreground_dilated.ptr<uchar>(j, i)[0] > 0)
						{
							fill_image_foreground_dilated = true;
							i = WIDTH_SMALL;
							break;
						}

				if (fill_image_foreground_dilated)
					for (int i = 0; i < WIDTH_SMALL; ++i)
						for (int j = 0; j < HEIGHT_SMALL; ++j)
							if (image_foreground_dilated.ptr<uchar>(j, i)[0] == 0)
								fill_image_background_static(i, j, image_background);

				uchar static_diff_max = 0;

				for (int i = 0; i < WIDTH_SMALL; ++i)
					for (int j = 0; j < HEIGHT_SMALL; ++j)
						if (image_in_thresholded.ptr<uchar>(j, i)[0] > 0)
							if (image_background_static.ptr<uchar>(j, i)[0] < 255)
							{
								const uchar diff = abs(image_in.ptr<uchar>(j, i)[0] - image_background_static.ptr<uchar>(j, i)[0]);
								if (diff > static_diff_max)
									static_diff_max = diff;
							}

				static float diff_threshold_stereo;
				if (name == motion_processor_primary_name)
				{
					diff_threshold = static_diff_max * 0.3;
					low_pass_filter->compute(diff_threshold, 0.1, "diff_threshold");

					diff_threshold_stereo = diff_threshold;
				}
				else
					diff_threshold = diff_threshold_stereo;

				vector<int> hit_x_min_vec;
				vector<int> hit_x_max_vec;

				for (int j = 0; j < HEIGHT_SMALL; ++j)
				{
					int hit_x_min = 9999;
					int hit_x_max = 0;
					for (int i = 0; i < WIDTH_SMALL; ++i)
						if (image_foreground.ptr<uchar>(j, i)[0] == 254)
						{
							if (i < hit_x_min)
								hit_x_min = i;
							if (i > hit_x_max)
								hit_x_max = i;
						}
					hit_x_min_vec.push_back(hit_x_min);
					hit_x_max_vec.push_back(hit_x_max);
				}

				sort(hit_x_min_vec.begin(), hit_x_min_vec.end());
				sort(hit_x_max_vec.begin(), hit_x_max_vec.end());

				int hit_x_min_final = hit_x_min_vec[hit_x_min_vec.size() * 0.1];
				int hit_x_max_final = hit_x_max_vec[hit_x_max_vec.size() * 0.9];

				//triangle fill
				Point pt_center = Point(x_separator_middle_median, y_separator_motion_up_median);
				if (pt_center.y >= 5)
				{
					Mat image_flood_fill = Mat::zeros(pt_center.y + 1, WIDTH_SMALL, CV_8UC1);
					line(image_flood_fill, pt_center, Point(hit_x_min_final, 0), Scalar(254), 1);
					line(image_flood_fill, pt_center, Point(hit_x_max_final, 0), Scalar(254), 1);
					floodFill(image_flood_fill, Point(pt_center.x, 0), Scalar(254));

					const int j_max = image_flood_fill.rows;
					for (int i = 0; i < WIDTH_SMALL; ++i)
						for (int j = 0; j < j_max; ++j)
							if (image_flood_fill.ptr<uchar>(j, i)[0] > 0)
								fill_image_background_static(i, j, image_in);
				}

				if (value_store.get_bool("both_hands_are_moving"))
				{
					value_store.set_int("target_frame", 1);
					value_store.set_bool("result", true);
				}

				if (visualize && enable_imshow)
				{
					imshow("image_in_thresholded" + name, image_in_thresholded);
					// imshow("image_subtraction", image_subtraction);
					imshow("image_background_static" + name, image_background_static);
					// imshow("image_foreground_motion_processor_new" + name, image_foreground);
				}
			}
		}
		else
		{
			vector<int> blob_count_vec;
			for (BlobNew& blob : *(blob_detector_image_subtraction->blobs))
				blob_count_vec.push_back(blob.count);

			sort(blob_count_vec.begin(), blob_count_vec.end());
			noise_size = blob_count_vec[blob_count_vec.size() * 0.5] * 4;
			low_pass_filter->compute(noise_size, 0.1, "noise_size");
		}

		value_store.set_mat("image_background", image_in);
	}
				
	value_store.set_int("current_frame", value_store.get_int("current_frame") + 1);

	if (value_store.get_bool("image_background_set") == false)
		value_store.set_mat("image_background", image_in);

	value_store.set_bool("image_background_set", true);

	if (dilated0 && dilated1)
		large_dilation = false;

	return value_store.get_bool("result");
}

bool MotionProcessorNew::compute_y_separator_motion()
{
	BlobDetectorNew* blob_detector_image_subtraction = value_store.get_blob_detector("blob_detector_image_subtraction");

	if (x_separator_middle == -1 || blob_detector_image_subtraction->blobs->size() == 0)
		return false;

	int y_min = 9999;
	int y_max = 0;
	for (BlobNew& blob : *blob_detector_image_subtraction->blobs)
		if (blob.active && blob.y < REFLECTION_Y)
		{
			if (blob.y_min < y_min)
				y_min = blob.y_min;
			if (blob.y_max > y_max)
				y_max = blob.y_max;
		}

	Mat image_foreground = value_store.get_mat("image_foreground");

	bool repeat = true;
	while (y_max < HEIGHT_SMALL && repeat)
	{
		repeat = false;
		for (int i = 0; i < WIDTH_SMALL; ++i)
			if (image_foreground.ptr<uchar>(y_max, i)[0] > 0)
			{
				repeat = true;
				++y_max;
				break;
			}
	}

	bool b0 = false;
	bool b1 = false;

	vector<int>* y_separator_motion_down_vec = value_store.get_int_vec("y_separator_motion_down_vec");

	if (y_separator_motion_down_vec->size() < 1000)
		y_separator_motion_down_vec->push_back(y_max);
	else
		(*y_separator_motion_down_vec)[y_separator_motion_down_vec->size() - 1] = y_max;

	if (y_separator_motion_down_vec->size() >= 1)
	{
		sort(y_separator_motion_down_vec->begin(), y_separator_motion_down_vec->end());
		y_separator_motion_down_median = (*y_separator_motion_down_vec)[y_separator_motion_down_vec->size() / 2];
		b0 = true;
	}

	vector<int>* y_separator_motion_up_vec = value_store.get_int_vec("y_separator_motion_up_vec");

	if (y_separator_motion_up_vec->size() < 1000)
		y_separator_motion_up_vec->push_back(y_min);
	else
		(*y_separator_motion_up_vec)[y_separator_motion_up_vec->size() - 1] = y_min;

	if (y_separator_motion_up_vec->size() >= 1)
	{
		sort(y_separator_motion_up_vec->begin(), y_separator_motion_up_vec->end());
		y_separator_motion_up_median = (*y_separator_motion_up_vec)[y_separator_motion_up_vec->size() / 2];
		b1 = true;
	}

	return b0 && b1;
}

bool MotionProcessorNew::compute_x_separator_middle()
{
	BlobDetectorNew* blob_detector_image_subtraction = value_store.get_blob_detector("blob_detector_image_subtraction");

	Mat image_active_blobs = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
	for (BlobNew& blob : *blob_detector_image_subtraction->blobs)
		if (blob.active)
			blob.fill(image_active_blobs, 254);

	Mat image_motion_blocks = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
	for (int i = 0; i < WIDTH_SMALL; ++i)
		for (int j = HEIGHT_SMALL_MINUS; j >= 0; --j)
			if (image_active_blobs.ptr<uchar>(j, i)[0] > 0)
			{
				line(image_motion_blocks, Point(i, j), Point(i, 0), Scalar(254), 1);
				break;
			}

	Mat image_motion_blocks_small;
	resize(image_motion_blocks, image_motion_blocks_small, Size(WIDTH_MIN, HEIGHT_MIN), 0, 0, INTER_LINEAR);

	BlobDetectorNew* blob_detector_image_motion_blocks = value_store.get_blob_detector("blob_detector_image_motion_blocks");

	GaussianBlur(image_motion_blocks_small, image_motion_blocks_small, Size(29, 1), 0, 0);
	threshold(image_motion_blocks_small, image_motion_blocks_small, 100, 254, THRESH_BINARY);
	blob_detector_image_motion_blocks->compute(image_motion_blocks_small, 254, 0, WIDTH_SMALL, 0, HEIGHT_SMALL, true);

	if (blob_detector_image_motion_blocks->blobs->size() >= 2 && blob_detector_image_motion_blocks->y_max_result > (HEIGHT_MIN / 5))
	{
		blob_detector_image_motion_blocks->sort_blobs_by_count();

		if (((float)(*blob_detector_image_motion_blocks->blobs)[1].count) / (*blob_detector_image_motion_blocks->blobs)[0].count > 0.5)
		{
			vector<int>* x_separator_middle_vec = value_store.get_int_vec("x_separator_middle_vec");

			if (blob_detector_image_motion_blocks->blobs->size() == 2)
			{
				x_separator_middle = (*blob_detector_image_motion_blocks->blobs)[0].x + (*blob_detector_image_motion_blocks->blobs)[1].x;

				if (x_separator_middle_vec->size() < 1000)
					x_separator_middle_vec->push_back(x_separator_middle);
				else
					(*x_separator_middle_vec)[x_separator_middle_vec->size() - 1] = x_separator_middle;
			}

			if (x_separator_middle_vec->size() >= 1)
			{
				sort(x_separator_middle_vec->begin(), x_separator_middle_vec->end());
				x_separator_middle_median = (*x_separator_middle_vec)[x_separator_middle_vec->size() / 2];
			
				if (value_store.get_int("both_hands_are_moving_count") >= 1)
				{
					value_store.set_bool("both_hands_are_moving", true);
					value_store.set_bool("compute_x_separator_middle_result", true);
				}

				value_store.set_int("both_hands_are_moving_count", value_store.get_int("both_hands_are_moving_count") + 1);
			}
		}
		else
			value_store.set_int("both_hands_are_moving_count", 0);
	}
	else
		value_store.set_int("both_hands_are_moving_count", 0);

	return value_store.get_bool("compute_x_separator_middle_result");
}

bool MotionProcessorNew::compute_x_separator_motion_left_right()
{
	if (value_store.get_bool("both_hands_are_moving") == false)
		return false;

	BlobDetectorNew* blob_detector_image_subtraction = value_store.get_blob_detector("blob_detector_image_subtraction");

	int x_min = 9999;
	int x_max = -1;
	for (BlobNew& blob : *blob_detector_image_subtraction->blobs)
		if (blob.active)
		{
			if (blob.x_min < x_min)
				x_min = blob.x_min;

			if (blob.x_max > x_max)
				x_max = blob.x_max;
		}

	if (x_min < 9999)
	{
		vector<int>* x_separator_motion_left_vec = value_store.get_int_vec("x_separator_motion_left_vec");

		if (x_separator_motion_left_vec->size() < 1000)
			x_separator_motion_left_vec->push_back(x_min);
		else
			(*x_separator_motion_left_vec)[x_separator_motion_left_vec->size() - 1] = x_min;

		sort(x_separator_motion_left_vec->begin(), x_separator_motion_left_vec->end());
		x_separator_motion_left_median = (*x_separator_motion_left_vec)[x_separator_motion_left_vec->size() * 0.5];
	}

	if (x_max > -1)
	{
		vector<int>* x_separator_motion_right_vec = value_store.get_int_vec("x_separator_motion_right_vec");

		if (x_separator_motion_right_vec->size() < 1000)
			x_separator_motion_right_vec->push_back(x_max);
		else
			(*x_separator_motion_right_vec)[x_separator_motion_right_vec->size() - 1] = x_max;

		sort(x_separator_motion_right_vec->begin(), x_separator_motion_right_vec->end());
		x_separator_motion_right_median = (*x_separator_motion_right_vec)[x_separator_motion_right_vec->size() * 0.5];
	}

	return true;
}

inline void MotionProcessorNew::fill_image_background_static(const int x, const int y, Mat& image_in)
{
	uchar* pix_ptr = &(image_background_static.ptr<uchar>(y, x)[0]);

	if (*pix_ptr == 255)
		*pix_ptr = image_in.ptr<uchar>(y, x)[0];
	else
		*pix_ptr = *pix_ptr + ((image_in.ptr<uchar>(y, x)[0] - *pix_ptr) * 0.25);
}

void MotionProcessorNew::compute_slope(Mat& image_in_thresholded, string name)
{
	int x_min;
	int x_max;
	if (name == "left")
	{
		x_min = 0;
		x_max = x_separator_motion_left_median;
	}
	else
	{
		x_min = x_separator_motion_right_median;
		x_max = WIDTH_SMALL_MINUS;
	}

	const int x_min_const = x_min;
	const int x_max_const = x_max;

	vector<Point> pt_intersection_top_vec;
	vector<Point> pt_intersection_bottom_vec;

	int offset_count = 0;
	for (int j_offset = REFLECTION_Y - y_separator_motion_down_median; j_offset >= 0; --j_offset)
	{
		Point pt_old = Point(-1, -1);
		for (int i = x_min_const; i <= x_max_const; ++i)
			for (int j = REFLECTION_Y - j_offset; j >= 0; --j)
				if (image_in_thresholded.ptr<uchar>(j, i)[0] > 0)
				{
					Point pt_new = Point(i, j);
					if (pt_old.x != -1)
					{
						Point pt_intersection_top;
						Point pt_intersection_bottom;
						bool b0 = get_intersection_at_y(pt_new, pt_old, 0, pt_intersection_top);
						bool b1 = get_intersection_at_y(pt_new, pt_old, HEIGHT_SMALL_MINUS, pt_intersection_bottom);

						if (b0 && b1)
							if (name == "left" && pt_intersection_top.x > pt_intersection_bottom.x ||
								name == "right" && pt_intersection_top.x < pt_intersection_bottom.x)
							{
								pt_intersection_top_vec.push_back(pt_intersection_top);
								pt_intersection_bottom_vec.push_back(pt_intersection_bottom);
							}
				}
				pt_old = pt_new;
				break;
			}
		++offset_count;
	}

	if (pt_intersection_top_vec.size() <= 5 * offset_count)
		return;

	sort(pt_intersection_top_vec.begin(), pt_intersection_top_vec.end(), compare_point_x());
	sort(pt_intersection_bottom_vec.begin(), pt_intersection_bottom_vec.end(), compare_point_x());

	Point pt_top = pt_intersection_top_vec[pt_intersection_top_vec.size() * 0.5];
	Point pt_bottom = pt_intersection_bottom_vec[pt_intersection_bottom_vec.size() * 0.5];

	value_store.set_point("pt_top_" + name, pt_top);
	value_store.set_point("pt_bottom_" + name, pt_bottom);
}