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

bool MotionProcessorNew::compute(Mat& image_in, Mat& image_raw_in, const string name, const bool visualize)
{
	if (mode == "tool")
		return false;

	static string motion_processor_primary_name = "";
	if (motion_processor_primary_name == "")
		motion_processor_primary_name = name;

	value_store.set_bool("left_hand_is_moving", false);
	value_store.set_bool("right_hand_is_moving", false);

	static bool set_both_hands_are_moving = false;
	if (name != motion_processor_primary_name && set_both_hands_are_moving)
		value_store.set_bool("both_hands_are_moving", true);
	else
		value_store.set_bool("both_hands_are_moving", false);

	Mat image_background = value_store.get_mat("image_background");

	LowPassFilter* low_pass_filter = value_store.get_low_pass_filter("low_pass_filter");

	if (value_store.get_bool("image_background_set") &&
		value_store.get_int("current_frame") >= value_store.get_int("target_frame", 5))
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

		motion_state = 0;
		if (blob_detector_image_subtraction->blobs->size() < 100 && blob_detector_image_subtraction->blobs->size() > 0)
		{
			motion_state = 1;

			int intensity_array[WIDTH_SMALL] { 0 };
			for (BlobNew& blob : *blob_detector_image_subtraction->blobs)
				for (Point& pt : blob.data)
					++intensity_array[pt.x];

			int x_min = 9999;
			int x_max = 0;

			vector<Point> hist_pt_vec;
			for (int i = 0; i < WIDTH_SMALL; ++i)
			{
				int j = intensity_array[i];
				low_pass_filter->compute(j, 0.5, "histogram_j");

				if (j < 10)
					continue;

				for (int j_current = 0; j_current < j; ++j_current)
					hist_pt_vec.push_back(Point(i, j_current));

				if (i < x_min)
					x_min = i;
				if (i > x_max)
					x_max = i;
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

			if (seed_vec0.size() == 0 || seed_vec1.size() == 0)
				return false;

			float x_seed_vec0_max = seed_vec0[seed_vec0.size() - 1].x;
			float x_seed_vec1_min = seed_vec1[0].x;

			low_pass_filter->compute_if_smaller(x_seed_vec0_max, 0.5, "x_seed_vec0_max");
			low_pass_filter->compute_if_larger(x_seed_vec1_min, 0.5, "x_seed_vec1_min");

			if (false) //for visualization
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

			float hand_width0 = x_seed_vec0_max - x_min;
			float hand_width1 = x_max - x_seed_vec1_min;
			float hand_width_max = max(hand_width0, hand_width1);
			float gap_width = x_seed_vec1_min - x_seed_vec0_max;
			float total_width = x_max - x_min;

			bool both_hands_are_moving = (total_width > 80 && (gap_width / hand_width_max) > 0.3);

			int blobs_count_left = 0;
			int blobs_count_right = 0;
			for (BlobNew& blob : *blob_detector_image_subtraction->blobs)
				if (blob.x < x_separator_middle)
					blobs_count_left += blob.count;
				else
					blobs_count_right += blob.count;

			bool left_hand_is_moving = blobs_count_left / x_separator_middle >= 10 || blobs_count_left >= 500;
			bool right_hand_is_moving = blobs_count_right / (WIDTH_SMALL - x_separator_middle) >= 10 || blobs_count_right >= 500;

			if (both_hands_are_moving)
			{
				left_hand_is_moving = true;
				right_hand_is_moving = true;
			}

			if (left_hand_is_moving == false)
				value_store.set_bool("left_hand_is_moving_false", true);

			if (right_hand_is_moving == false)
				value_store.set_bool("right_hand_is_moving_false", true);

			if (value_store.get_bool("left_hand_is_moving_false") && value_store.get_bool("right_hand_is_moving_false") &&
				(left_hand_is_moving || right_hand_is_moving || both_hands_are_moving))
			{
				if (both_hands_are_moving)
				{
					x_separator_middle = (x_seed_vec1_min + x_seed_vec0_max) / 2;
					value_store.set_bool("x_separator_middle_set", true);
				}
				else if (left_hand_is_moving && !value_store.get_bool("x_separator_middle_set"))
				{
					x_separator_middle = x_max + 10;
					value_store.set_bool("x_separator_middle_set", true);
				}
				else if (right_hand_is_moving && !value_store.get_bool("x_separator_middle_set"))
				{
					x_separator_middle = x_min - 10;
					value_store.set_bool("x_separator_middle_set", true);
				}

				int intensity_array0[HEIGHT_SMALL] { 0 };
				for (BlobNew& blob : *blob_detector_image_subtraction->blobs)
					for (Point& pt : blob.data)
						++intensity_array0[pt.y];

				int y_min = 9999;
				int y_max = 0;

				vector<Point> hist_pt_vec0;
				for (int i = 0; i < HEIGHT_SMALL; ++i)
				{
					int j = intensity_array0[i];
					low_pass_filter->compute(j, 0.5, "histogram_j");

					if (j < 10)
						continue;

					for (int j_current = 0; j_current < j; ++j_current)
						hist_pt_vec0.push_back(Point(j_current, i));

					if (i < y_min)
						y_min = i;
					if (i > y_max)
						y_max = i;
				}

				Mat image_histogram = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
				for (Point& pt : hist_pt_vec0)
					line(image_histogram, pt, Point(0, pt.y), Scalar(254), 1);

				BlobDetectorNew* blob_detector_image_histogram = value_store.get_blob_detector("blob_detector_image_histogram");
				blob_detector_image_histogram->compute(image_histogram, 254, 0, WIDTH_SMALL, 0, HEIGHT_SMALL, true);

				if (both_hands_are_moving)
				{
					y_separator_motion_down_median = blob_detector_image_histogram->blob_max_size->y_max;
					value_store.set_int("y_separator_motion_down_median_left", y_separator_motion_down_median);
					value_store.set_int("y_separator_motion_down_median_right", y_separator_motion_down_median);
				}
				else if (left_hand_is_moving)
				{
					int y_separator_motion_down_median_left = blob_detector_image_histogram->blob_max_size->y_max;
					int y_separator_motion_down_median_right = value_store.get_int("y_separator_motion_down_median_right", 0);
					value_store.set_int("y_separator_motion_down_median_left", y_separator_motion_down_median_left);

					if (y_separator_motion_down_median_left > y_separator_motion_down_median_right)
						y_separator_motion_down_median = y_separator_motion_down_median_left;
				}
				else if (right_hand_is_moving)
				{
					int y_separator_motion_down_median_right = blob_detector_image_histogram->blob_max_size->y_max;
					int y_separator_motion_down_median_left = value_store.get_int("y_separator_motion_down_median_left", 0);
					value_store.set_int("y_separator_motion_down_median_right", y_separator_motion_down_median_right);

					if (y_separator_motion_down_median_right > y_separator_motion_down_median_left)
						y_separator_motion_down_median = y_separator_motion_down_median_right;
				}

				static float gray_threshold_left_stereo = 9999;
				static float gray_threshold_right_stereo = 9999;

				if (name == motion_processor_primary_name)
				{
					vector<uchar> gray_vec_left;
					vector<uchar> gray_vec_right;

					for (BlobNew& blob : *(blob_detector_image_subtraction->blobs))
						if (blob.active)
							if (blob.x < x_separator_middle)
							{
								for (Point pt : blob.data)
								{
									uchar gray = std::max(image_in.ptr<uchar>(pt.y, pt.x)[0], image_background.ptr<uchar>(pt.y, pt.x)[0]);
									gray_vec_left.push_back(gray);
								}
							}
							else
							{
								for (Point pt : blob.data)
								{
									uchar gray = std::max(image_in.ptr<uchar>(pt.y, pt.x)[0], image_background.ptr<uchar>(pt.y, pt.x)[0]);
									gray_vec_right.push_back(gray);
								}	
							}

					if (gray_vec_left.size() > 0 && left_hand_is_moving)
					{
						sort(gray_vec_left.begin(), gray_vec_left.end());
						float gray_median_left = gray_vec_left[gray_vec_left.size() * 0.5];

						gray_threshold_left = gray_median_left - 64;
						low_pass_filter->compute(gray_threshold_left, 0.1, "gray_threshold_left");
						gray_threshold_left_stereo = gray_threshold_left;
					}

					if (gray_vec_right.size() > 0 && right_hand_is_moving)
					{
						sort(gray_vec_right.begin(), gray_vec_right.end());
						float gray_median_right = gray_vec_right[gray_vec_right.size() * 0.5];

						gray_threshold_right = gray_median_right - 64;
						low_pass_filter->compute(gray_threshold_right, 0.1, "gray_threshold_right");
						gray_threshold_right_stereo = gray_threshold_right;
					}
				}
				else
				{
					gray_threshold_left = gray_threshold_left_stereo;
					gray_threshold_right = gray_threshold_right_stereo;
				}

				Mat image_in_thresholded = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
				for (int i = 0; i < WIDTH_SMALL; ++i)
					for (int j = 0; j < HEIGHT_SMALL; ++j)
						if (i < x_separator_middle && image_in.ptr<uchar>(j, i)[0] > gray_threshold_left)
							image_in_thresholded.ptr<uchar>(j, i)[0] = 254;
						else if (i > x_separator_middle && image_in.ptr<uchar>(j, i)[0] > gray_threshold_right)
							image_in_thresholded.ptr<uchar>(j, i)[0] = 254;

				// for (BlobNew& blob : *(blob_detector_image_subtraction->blobs))
				// 	if ((y_separator_motion_down_median > -1 && blob.y > y_separator_motion_down_median) || blob.count < noise_size)
				// 	{
				// 		blob.active = false;
				// 		blob.fill(image_subtraction, 0);
				// 	}

				line(image_in_thresholded, Point(x_separator_middle, 0), Point(x_separator_middle, 9999), Scalar(254), 1);
				line(image_in_thresholded, Point(x_separator_middle, 0), Point(x_separator_middle, 9999), Scalar(254), 1);
				line(image_in_thresholded, Point(0, y_separator_motion_down_median), Point(9999, y_separator_motion_down_median), Scalar(254), 1);

				imshow("image_subtractionTTT" + name, image_subtraction);
				imshow("image_in_thresholdedTTT" + name, image_in_thresholded);
			}




			/*for (BlobNew& blob : *(blob_detector_image_subtraction->blobs))
				if (blob.count < noise_size || (y_separator_motion_down_median > -1 && blob.y > y_separator_motion_down_median))
				{
					blob.active = false;
					blob.fill(image_subtraction, 0);
				}

			if (!value_store.get_bool("separators_computed"))
			{
				if (value_store.get_int("separator_timer") > 10)
				{
					value_store.set_bool("reset_separators", true);
					value_store.set_int("separator_timer", 0);
				}
				else
					value_store.set_bool("reset_separators", false);

				bool proceed0 = compute_x_separator_middle();
				bool proceed1 = compute_x_separator_motion_left_right();
				bool proceed2 = compute_y_separator_motion();

				// COUT << proceed0 << " " << proceed1 << " " << proceed2 << endl;

				if (value_store.get_bool("both_hands_are_moving"))
					value_store.set_int("separator_timer", 0);
				else
					value_store.set_int("separator_timer", value_store.get_int("separator_timer") + 1);

				if (proceed0 && proceed1 && proceed2)
					value_store.set_bool("separators_computed", true);
			}
			else
				compute_x_separator_middle();

			const int push_count_max = 5;
			if (value_store.get_int("push_count") < push_count_max)
				if (value_store.get_bool("both_hands_are_moving"))
				{
					vector<int>* x_vec = value_store.get_int_vec("x_vec");
					vector<int>* y_vec = value_store.get_int_vec("y_vec");

					for (BlobNew& blob : *blob_detector_image_subtraction->blobs)
						for (Point& pt : blob.data)
						{
							x_vec->push_back(pt.x);
							y_vec->push_back(pt.y);
						}

					value_store.set_int("push_count", value_store.get_int("push_count") + 1);
				}

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
                        {
							if (blob.x < x_separator_middle_median)
								++left_count;
							else
								++right_count;
                        }

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
                        {
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
				}

				static float gray_threshold_max = 0;
				static float gray_threshold_left_stereo = 0;
				static float gray_threshold_right_stereo = 0;

				if (name == motion_processor_primary_name)
				{
					vector<uchar> gray_vec_left;
					vector<uchar> gray_vec_right;

					for (BlobNew& blob : *(blob_detector_image_subtraction->blobs))
						if (blob.active)
							if (blob.x < x_separator_middle_median)
							{
								for (Point pt : blob.data)
								{
									uchar gray = std::max(image_in.ptr<uchar>(pt.y, pt.x)[0], image_background.ptr<uchar>(pt.y, pt.x)[0]);
									gray_vec_left.push_back(gray);
								}
							}
							else
							{
								for (Point pt : blob.data)
								{
									uchar gray = std::max(image_in.ptr<uchar>(pt.y, pt.x)[0], image_background.ptr<uchar>(pt.y, pt.x)[0]);
									gray_vec_right.push_back(gray);
								}	
							}

					float gray_threshold_left_old = gray_threshold_left;
					float gray_threshold_right_old = gray_threshold_right;

					if (gray_vec_left.size() > 0)
					{
						sort(gray_vec_left.begin(), gray_vec_left.end());
						float gray_median_left = gray_vec_left[gray_vec_left.size() * 0.5];

						gray_threshold_left = gray_median_left - 20;
						low_pass_filter->compute(gray_threshold_left, 0.1, "gray_threshold_left");
						gray_threshold_left_stereo = gray_threshold_left;
					}

					if (gray_vec_right.size() > 0)
					{
						sort(gray_vec_right.begin(), gray_vec_right.end());
						float gray_median_right = gray_vec_right[gray_vec_right.size() * 0.5];

						gray_threshold_right = gray_median_right - 20;
						low_pass_filter->compute(gray_threshold_right, 0.1, "gray_threshold_right");
						gray_threshold_right_stereo = gray_threshold_right;
					}

					float gray_threshold_max_current = max(gray_threshold_left, gray_threshold_right);
					if (gray_threshold_max_current > gray_threshold_max && value_store.get_bool("both_hands_are_moving"))
						gray_threshold_max = gray_threshold_max_current;

					if ((float)gray_threshold_left / gray_threshold_max < 0.8)
					{
						gray_threshold_left = gray_threshold_left_old;
						gray_threshold_left_stereo = gray_threshold_left_old;
					}

					if ((float)gray_threshold_right / gray_threshold_max < 0.8)
					{
						gray_threshold_right = gray_threshold_right_old;
						gray_threshold_right_stereo = gray_threshold_right_old;
					}
				}
				else
				{
					gray_threshold_left = gray_threshold_left_stereo;
					gray_threshold_right = gray_threshold_right_stereo;
				}

				Mat image_in_thresholded = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
				for (int i = 0; i < WIDTH_SMALL; ++i)
					for (int j = 0; j < HEIGHT_SMALL; ++j)
						if (i < x_separator_middle_median && image_in.ptr<uchar>(j, i)[0] > gray_threshold_left)
							image_in_thresholded.ptr<uchar>(j, i)[0] = 254;
						else if (i > x_separator_middle_median && image_in.ptr<uchar>(j, i)[0] > gray_threshold_right)
							image_in_thresholded.ptr<uchar>(j, i)[0] = 254;

				if (value_store.get_int("push_count") == push_count_max)
				{
					vector<int>* x_vec = value_store.get_int_vec("x_vec");
					vector<int>* y_vec = value_store.get_int_vec("y_vec");

					sort(x_vec->begin(), x_vec->end());
					sort(y_vec->begin(), y_vec->end());

					Point pt0 = Point((*x_vec)[x_vec->size() * 0.25], (*y_vec)[y_vec->size() * 0.1]);
					Point pt1 = Point((*x_vec)[x_vec->size() * 0.1], (*y_vec)[y_vec->size() * 0.9]);

					int x_diff0 = ((pt0.x + pt1.x) / 2) - x_separator_motion_left_median;		
					pt0.x -= x_diff0;
					pt1.x -= x_diff0;

					Point pt2 = Point((*x_vec)[x_vec->size() * 0.75], (*y_vec)[y_vec->size() * 0.1]);
					Point pt3 = Point((*x_vec)[x_vec->size() * 0.9], (*y_vec)[y_vec->size() * 0.9]);

					int x_diff1 = x_separator_motion_right_median - ((pt2.x + pt3.x) / 2);
					pt2.x += x_diff1;
					pt3.x += x_diff1;

					x_vec->clear();
					y_vec->clear();

					Point pt_top0;
					Point pt_top1;
					Point pt_bottom0;
					Point pt_bottom1;

					bool bi0 = get_intersection_at_y(pt0, pt1, 0, pt_top0);
					bool bi1 = get_intersection_at_y(pt2, pt3, 0, pt_top1);
					bool bi2 = get_intersection_at_y(pt0, pt1, HEIGHT_SMALL, pt_bottom0);
					bool bi3 = get_intersection_at_y(pt2, pt3, HEIGHT_SMALL, pt_bottom1);
					bool bi4 = pt_top0.x > 0;
					bool bi5 = pt_top1.x < WIDTH_SMALL_MINUS;
					bool bi6 = x_separator_motion_left_median > 0;
					bool bi7 = x_separator_motion_right_median < WIDTH_SMALL_MINUS;

					if (bi0 && bi1 && bi2 && bi3 && bi4 && bi5 && bi6 && bi7)
					{
						Mat image_borders = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);

						line(image_borders, pt_top0, pt_bottom0, Scalar(254), 1);
						line(image_borders, pt_top1, pt_bottom1, Scalar(254), 1);

						floodFill(image_borders, Point(0, 0), Scalar(254));
						floodFill(image_borders, Point(WIDTH_SMALL_MINUS, 0), Scalar(254));

						line(image_borders, Point(x_separator_motion_left_median, 0), 
										    Point(x_separator_motion_left_median, 9999), Scalar(254), 1);

						line(image_borders, Point(x_separator_motion_right_median, 0), 
									        Point(x_separator_motion_right_median, 9999), Scalar(254), 1);

						floodFill(image_borders, Point(x_separator_motion_left_median - 1, HEIGHT_SMALL_MINUS), Scalar(254));
						floodFill(image_borders, Point(x_separator_motion_right_median + 1, HEIGHT_SMALL_MINUS), Scalar(254));

						line(image_borders, Point(0, y_separator_motion_down_median),
										    Point(9999, y_separator_motion_down_median), Scalar(254), 1);

						floodFill(image_borders,
								  Point((x_separator_motion_left_median + x_separator_motion_right_median) / 2, HEIGHT_SMALL_MINUS),
								  Scalar(254));

						for (int i = 0; i < WIDTH_SMALL; ++i)
							for (int j = 0; j < HEIGHT_SMALL; ++j)
								if (image_borders.ptr<uchar>(j, i)[0] > 0)
									fill_image_background_static(i, j, image_in);
					}

					value_store.set_int("push_count", 9999);
					value_store.set_bool("slopes_computed", true);
				}

				Mat image_in_thresholded_dilated;
				if (value_store.get_bool("first_dilation", false) == false)
					dilate(image_in_thresholded, image_in_thresholded_dilated, Mat(), Point(-1, -1), 20);
				else
					dilate(image_in_thresholded, image_in_thresholded_dilated, Mat(), Point(-1, -1), 5);

				value_store.set_bool("first_dilation", true);

				for (int i = 0; i < WIDTH_SMALL; ++i)
					for (int j = 0; j < HEIGHT_SMALL; ++j)
						if (image_in_thresholded_dilated.ptr<uchar>(j, i)[0] == 0)
							fill_image_background_static(i, j, image_in);

				uchar static_diff_max = 0;
				uchar static_diff_max_left = 0;
				uchar static_diff_max_right = 0;

				for (int i = 0; i < WIDTH_SMALL; ++i)
					for (int j = 0; j < HEIGHT_SMALL; ++j)
						if (image_in_thresholded.ptr<uchar>(j, i)[0] > 0)
							if (image_background_static.ptr<uchar>(j, i)[0] < 255)
							{
								const uchar diff = abs(image_in.ptr<uchar>(j, i)[0] - image_background_static.ptr<uchar>(j, i)[0]);

								if (diff > static_diff_max)
									static_diff_max = diff;

								if (diff > static_diff_max_left && i < x_separator_middle_median)
									static_diff_max_left = diff;
								else if (diff > static_diff_max_right && i > x_separator_middle_median)
									static_diff_max_right = diff;
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

				//triangle fill
				Point pt_center = Point(x_separator_middle_median, y_separator_motion_up_median);
				if (pt_center.y >= 5)
				{
					Mat image_flood_fill = Mat::zeros(pt_center.y + 1, WIDTH_SMALL, CV_8UC1);
					line(image_flood_fill, pt_center, Point(x_separator_motion_left_median, 0), Scalar(254), 1);
					line(image_flood_fill, pt_center, Point(x_separator_motion_right_median, 0), Scalar(254), 1);
					floodFill(image_flood_fill, Point(pt_center.x, 0), Scalar(254));

					const int j_max = image_flood_fill.rows;
					for (int i = 0; i < WIDTH_SMALL; ++i)
						for (int j = 0; j < j_max; ++j)
							if (image_flood_fill.ptr<uchar>(j, i)[0] > 0)
								fill_image_background_static(i, j, image_in);
				}

				if (value_store.get_bool("both_hands_are_moving") &&
					value_store.get_bool("set_result", true) &&
					value_store.get_bool("slopes_computed"))
				{
					value_store.set_bool("set_result", false);
					// value_store.set_int("target_frame", 10);
					value_store.set_bool("result", true);
				}

				if (visualize && enable_imshow)
				{
					Mat matt = image_background_static.clone();
					line(matt, Point(0, y_separator_motion_down_median), Point(9999, y_separator_motion_down_median), Scalar(255), 1);
					imshow("image_in_thresholded" + name, image_in_thresholded);
					// imshow("image_subtraction", image_subtraction);
					imshow("matt" + name, matt);
				}
			}*/
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

		if (value_store.get_bool("result"))
		{
			Mat image_foreground = compute_image_foreground(image_in);
			Mat image_masked = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);

			for (int i = 0; i < WIDTH_SMALL; ++i)
				for (int j = 0; j < HEIGHT_SMALL; ++j)
					if (image_foreground.ptr<uchar>(j, i)[0] > 0)
						image_masked.ptr<uchar>(j, i)[0] = image_raw_in.ptr<uchar>(j, i)[2];

			imshow("image_masked", image_masked);

			value_store.set_mat("image_foreground", image_foreground);
		}
	}
	
	value_store.set_int("current_frame", value_store.get_int("current_frame") + 1);

	if (value_store.get_bool("image_background_set") == false)
		value_store.set_mat("image_background", image_in);

	value_store.set_bool("image_background_set", true);

	if (name == motion_processor_primary_name && value_store.get_bool("both_hands_are_moving"))
		set_both_hands_are_moving = true;
	else
		set_both_hands_are_moving = false;

	return value_store.get_bool("result");
}

bool MotionProcessorNew::compute_y_separator_motion()
{
	vector<int>* y_separator_motion_down_vec = value_store.get_int_vec("y_separator_motion_down_vec");
	vector<int>* y_separator_motion_up_vec = value_store.get_int_vec("y_separator_motion_up_vec");
	if (value_store.get_bool("reset_separators"))
	{
		y_separator_motion_down_vec->clear();
		y_separator_motion_up_vec->clear();
	}

	BlobDetectorNew* blob_detector_image_subtraction = value_store.get_blob_detector("blob_detector_image_subtraction");

	if (x_separator_middle != -1 && blob_detector_image_subtraction->blobs->size() > 0 && value_store.get_bool("both_hands_are_moving"))
	{
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

		Mat image_foreground = value_store.get_mat("image_foreground", true);

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

		if (y_separator_motion_down_vec->size() < 1000)
			y_separator_motion_down_vec->push_back(y_max);
		else
			(*y_separator_motion_down_vec)[y_separator_motion_down_vec->size() - 1] = y_max;

		if (y_separator_motion_down_vec->size() >= vector_completion_size)
		{
			sort(y_separator_motion_down_vec->begin(), y_separator_motion_down_vec->end());
			y_separator_motion_down_median = (*y_separator_motion_down_vec)[y_separator_motion_down_vec->size() / 2];
		}

		if (y_separator_motion_up_vec->size() < 1000)
			y_separator_motion_up_vec->push_back(y_min);
		else
			(*y_separator_motion_up_vec)[y_separator_motion_up_vec->size() - 1] = y_min;

		if (y_separator_motion_up_vec->size() >= vector_completion_size)
		{
			sort(y_separator_motion_up_vec->begin(), y_separator_motion_up_vec->end());
			y_separator_motion_up_median = (*y_separator_motion_up_vec)[y_separator_motion_up_vec->size() / 2];
		}
	}

	return y_separator_motion_down_vec->size() >= vector_completion_size && y_separator_motion_up_vec->size() >= vector_completion_size;
}

bool MotionProcessorNew::compute_x_separator_middle()
{
	vector<int>* x_separator_middle_vec = value_store.get_int_vec("x_separator_middle_vec");
	if (value_store.get_bool("reset_separators"))
		x_separator_middle_vec->clear();

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

	if (blob_detector_image_motion_blocks->blobs->size() >= 2 && blob_detector_image_motion_blocks->y_max_result >= (HEIGHT_MIN / 5))
	{
		blob_detector_image_motion_blocks->sort_blobs_by_count();

		if (((float)(*blob_detector_image_motion_blocks->blobs)[1].count) / (*blob_detector_image_motion_blocks->blobs)[0].count > 0.3)
		{
			if (blob_detector_image_motion_blocks->blobs->size() == 2)
			{
				x_separator_middle = (*blob_detector_image_motion_blocks->blobs)[0].x + (*blob_detector_image_motion_blocks->blobs)[1].x;

				if (x_separator_middle_vec->size() < 1000)
					x_separator_middle_vec->push_back(x_separator_middle);
				else
					(*x_separator_middle_vec)[x_separator_middle_vec->size() - 1] = x_separator_middle;
			}

			if (x_separator_middle_vec->size() >= vector_completion_size)
			{
				sort(x_separator_middle_vec->begin(), x_separator_middle_vec->end());
				x_separator_middle_median = (*x_separator_middle_vec)[x_separator_middle_vec->size() / 2];
			
				if (value_store.get_int("both_hands_are_moving_count") >= 1)
					value_store.set_bool("both_hands_are_moving", true);

				value_store.set_int("both_hands_are_moving_count", value_store.get_int("both_hands_are_moving_count") + 1);
			}
		}
		else
			value_store.set_int("both_hands_are_moving_count", 0);
	}
	else
		value_store.set_int("both_hands_are_moving_count", 0);

	return x_separator_middle_vec->size() >= vector_completion_size;
}

bool MotionProcessorNew::compute_x_separator_motion_left_right()
{
	vector<int>* x_separator_motion_left_vec = value_store.get_int_vec("x_separator_motion_left_vec");
	vector<int>* x_separator_motion_right_vec = value_store.get_int_vec("x_separator_motion_right_vec");
	if (value_store.get_bool("reset_separators"))
	{
		x_separator_motion_left_vec->clear();
		x_separator_motion_right_vec->clear();
	}

	if (value_store.get_bool("both_hands_are_moving") == true)
	{
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

		if (x_min < 9999 && x_max > -1)
		{
			if (x_separator_motion_left_vec->size() < 1000)
				x_separator_motion_left_vec->push_back(x_min);
			else
				(*x_separator_motion_left_vec)[0] = x_min;

			if (x_separator_motion_right_vec->size() < 1000)
				x_separator_motion_right_vec->push_back(x_max);
			else
				(*x_separator_motion_right_vec)[0] = x_max;

			sort(x_separator_motion_left_vec->begin(), x_separator_motion_left_vec->end());
			x_separator_motion_left_median = (*x_separator_motion_left_vec)[x_separator_motion_left_vec->size() * 0.5];

			sort(x_separator_motion_right_vec->begin(), x_separator_motion_right_vec->end());
			x_separator_motion_right_median = (*x_separator_motion_right_vec)[x_separator_motion_right_vec->size() * 0.5];
		}
	}

	return x_separator_motion_left_vec->size() >= vector_completion_size && x_separator_motion_right_vec->size() >= vector_completion_size;
}

inline void MotionProcessorNew::fill_image_background_static(const int x, const int y, Mat& image_in)
{
	uchar* pix_ptr = &(image_background_static.ptr<uchar>(y, x)[0]);

	if (*pix_ptr == 255)
		*pix_ptr = image_in.ptr<uchar>(y, x)[0];
	else
		*pix_ptr = *pix_ptr + ((image_in.ptr<uchar>(y, x)[0] - *pix_ptr) * 0.25);
}

Mat MotionProcessorNew::compute_image_foreground(Mat& image_in)
{
	Mat image_foreground = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
	for (int i = 0; i < WIDTH_SMALL; ++i)
		for (int j = 0; j < HEIGHT_SMALL; ++j)
		{
			const uchar gray_current = image_in.ptr<uchar>(j, i)[0];

			if ((i <= x_separator_middle_median && gray_current > gray_threshold_left) ||
				(i > x_separator_middle_median && gray_current > gray_threshold_right))
			{
				if (image_background_static.ptr<uchar>(j, i)[0] == 255)
					image_foreground.ptr<uchar>(j, i)[0] = 254;
				else
					image_foreground.ptr<uchar>(j, i)[0] = abs(gray_current - image_background_static.ptr<uchar>(j, i)[0]);
			}
		}

	threshold(image_foreground, image_foreground, diff_threshold, 254, THRESH_BINARY);
	return image_foreground;
}