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

	motion_state = 0;

	static string motion_processor_primary_name = "";
	if (motion_processor_primary_name == "")
		motion_processor_primary_name = name;

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

		if (blob_detector_image_subtraction->blobs->size() < 100 && blob_detector_image_subtraction->blobs->size() > 0)
		{
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
			float gap_ratio = gap_width / hand_width_max;

			bool both_hands_are_moving = (total_width > 80 && gap_ratio > 0.3);

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

			if (left_hand_is_moving || right_hand_is_moving)
				motion_state = 1;

			static int compute_count = 0;
			if (compute_count < 10)
				++compute_count;

			if ((left_hand_is_moving || right_hand_is_moving) && compute_count >= 10)
			{
				if (both_hands_are_moving)
				{
					x_separator_left = x_min;
					x_separator_right = x_max;

					x_separator_middle = (x_seed_vec1_min + x_seed_vec0_max) / 2;
					value_store.set_bool("x_separator_middle_set", true);
				}
				else if (left_hand_is_moving)
				{
					x_separator_left = x_min;

					if (!value_store.get_bool("x_separator_middle_set"))
					{
						x_separator_middle = x_max + 20;
						value_store.set_bool("x_separator_middle_set", true);
					}
				}
				else if (right_hand_is_moving)
				{
					x_separator_right = x_max;

					if (!value_store.get_bool("x_separator_middle_set"))
					{
						x_separator_middle = x_min - 20;
						value_store.set_bool("x_separator_middle_set", true);
					}
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
					y_separator_down = blob_detector_image_histogram->blob_max_size->y_max;
					value_store.set_int("y_separator_down_left", y_separator_down);
					value_store.set_int("y_separator_down_right", y_separator_down);
				}
				else if (left_hand_is_moving)
				{
					int y_separator_down_left = blob_detector_image_histogram->blob_max_size->y_max;
					int y_separator_down_right = value_store.get_int("y_separator_down_right", 0);
					value_store.set_int("y_separator_down_left", y_separator_down_left);

					if (y_separator_down_left > y_separator_down_right)
						y_separator_down = y_separator_down_left;
				}
				else if (right_hand_is_moving)
				{
					int y_separator_down_right = blob_detector_image_histogram->blob_max_size->y_max;
					int y_separator_down_left = value_store.get_int("y_separator_down_left", 0);
					value_store.set_int("y_separator_down_right", y_separator_down_right);

					if (y_separator_down_right > y_separator_down_left)
						y_separator_down = y_separator_down_right;
				}

				low_pass_filter->compute(x_separator_left, 0.5, "x_separator_left");
				low_pass_filter->compute(x_separator_right, 0.5, "x_separator_right");
				low_pass_filter->compute(x_separator_middle, 0.5, "x_separator_middle");
				low_pass_filter->compute(y_separator_down, 0.5, "y_separator_down");

				static float gray_threshold_left_stereo = 9999;
				static float gray_threshold_right_stereo = 9999;

				if (name == motion_processor_primary_name)
				{
					vector<uchar> gray_vec_left;
					vector<uchar> gray_vec_right;

					for (BlobNew& blob : *(blob_detector_image_subtraction->blobs))
						if (blob.active)
                        {
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
								if (diff > static_diff_max_left && i < x_separator_middle)
									static_diff_max_left = diff;
								else if (diff > static_diff_max_right && i > x_separator_middle)
									static_diff_max_right = diff;
							}

				static float diff_threshold_stereo;
				if (name == motion_processor_primary_name)
				{
					diff_threshold = static_diff_max * 0.4;
					low_pass_filter->compute(diff_threshold, 0.1, "diff_threshold");
					diff_threshold_stereo = diff_threshold;
				}
				else
					diff_threshold = diff_threshold_stereo;

				Mat image_canny;
				Canny(image_raw_in, image_canny, 50, 50, 3);

				for (int i = 0; i < WIDTH_SMALL; ++i)
					for (int j = 0; j < HEIGHT_SMALL; ++j)
						if (image_canny.ptr<uchar>(j, i)[0] > 0)
							image_in_thresholded.ptr<uchar>(j, i)[0] = 0;

				for (BlobNew& blob : *(blob_detector_image_subtraction->blobs))
					if ((y_separator_down > -1 && blob.y > y_separator_down) || blob.count < noise_size)
					{
						blob.active = false;
						blob.fill(image_subtraction, 0);
					}
					else
						for (Point& pt : blob.data)
							if (image_in_thresholded.ptr<uchar>(pt.y, pt.x)[0] == 254)
								floodFill(image_in_thresholded, pt, Scalar(127));

				if (both_hands_are_moving)
				{
					for (int i = 0; i < WIDTH_SMALL; ++i)
						for (int j = 0; j < HEIGHT_SMALL; ++j)
							if (image_in_thresholded.ptr<uchar>(j, i)[0] == 254 &&
								(i < x_separator_left || i > x_separator_right || j > y_separator_down))
									image_in_thresholded.ptr<uchar>(j, i)[0] = 0;
				}
				else if (left_hand_is_moving)
				{
					for (int i = 0; i < WIDTH_SMALL; ++i)
						for (int j = 0; j < HEIGHT_SMALL; ++j)
							if (image_in_thresholded.ptr<uchar>(j, i)[0] == 254 &&
								(i < x_separator_left || j > y_separator_down))
									image_in_thresholded.ptr<uchar>(j, i)[0] = 0;
				}
				else if (right_hand_is_moving)
				{
					for (int i = 0; i < WIDTH_SMALL; ++i)
						for (int j = 0; j < HEIGHT_SMALL; ++j)
							if (image_in_thresholded.ptr<uchar>(j, i)[0] == 254 &&
								(i > x_separator_right || j > y_separator_down))
									image_in_thresholded.ptr<uchar>(j, i)[0] = 0;
				}

				dilate(image_in_thresholded, image_in_thresholded, Mat(), Point(-1, -1), 3);

				const int x_separator_middle_const = x_separator_middle;

				if (gray_threshold_left != 9999)
					for (int i = 0; i < x_separator_middle_const; ++i)
						for (int j = 0; j < HEIGHT_SMALL; ++j)
							if (image_in_thresholded.ptr<uchar>(j, i)[0] == 0)
								fill_image_background_static(i, j, image_in);

				if (gray_threshold_right != 9999)
					for (int i = x_separator_middle_const; i < WIDTH_SMALL; ++i)
						for (int j = 0; j < HEIGHT_SMALL; ++j)
							if (image_in_thresholded.ptr<uchar>(j, i)[0] == 0)
								fill_image_background_static(i, j, image_in);

				if (both_hands_are_moving)
				{
					int hit_y_max = -1;
					for (int j = 0; j < y_separator_down / 2; ++j)
						if (image_in_thresholded.ptr<uchar>(j, x_separator_middle)[0] == 254)
							hit_y_max = j;

					if (hit_y_max >= 0)
					{
						y_separator_up = hit_y_max;
						low_pass_filter->compute(y_separator_up, 0.5, "y_separator_up");

						const int y_separator_up_const = y_separator_up;
						for (int i = 0; i < WIDTH_SMALL; ++i)
							for (int j = 0; j <= y_separator_up_const; ++j)
								if (image_in_thresholded.ptr<uchar>(j, i)[0] == 254)
									fill_image_background_static(i, j, image_in);
					}

					value_store.set_bool("result", true);
				}


				if (visualize && enable_imshow)
				{
					line(image_in_thresholded, Point(x_separator_left, 0), Point(x_separator_left, 9999), Scalar(254), 1);
					line(image_in_thresholded, Point(x_separator_right, 0), Point(x_separator_right, 9999), Scalar(254), 1);
					line(image_in_thresholded, Point(x_separator_middle, 0), Point(x_separator_middle, 9999), Scalar(254), 1);
					line(image_in_thresholded, Point(0, y_separator_down), Point(9999, y_separator_down), Scalar(254), 1);

					imshow("image_subtractionTTT" + name, image_subtraction);
					imshow("image_in_thresholdedTTT" + name, image_in_thresholded);
					imshow("image_background_staticTTT" + name, image_background_static);
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

	return value_store.get_bool("result", false);
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
	return image_foreground;
}