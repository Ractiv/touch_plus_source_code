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

#include "mat_functions.h"
#include "console_log.h"

LowPassFilter mat_functions_low_pass_filter;
ValueStore mat_functions_value_store;

void threshold_get_bounds(Mat& image_in, Mat& image_out, const int threshold_val, int& x_min, int& x_max, int& y_min, int& y_max)
{
	Mat image_out_temp = Mat::zeros(image_in.size(), CV_8UC1);

	const int i_max = image_in.cols;
	const int j_max = image_in.rows;

	x_min = 9999;
	x_max = 0;
	y_min = 9999;
	y_max = 0;

	for (int i = 0; i < i_max; ++i)
		for (int j = 0; j < j_max; ++j)
			if (image_in.ptr<uchar>(j, i)[0] > threshold_val)
			{
				image_out_temp.ptr<uchar>(j, i)[0] = 254;

				if (i < x_min)
					x_min = i;
				if (i > x_max)
					x_max = i;
				if (j < y_min)
					y_min = j;
				if (j > y_max)
					y_max = j;
			}

	if (x_min == 9999)
		x_min = x_max = y_min = y_max = 0;

	image_out = image_out_temp;
}

Mat rotate_image(const Mat& image_in, const float angle, const Point origin, const int border)
{
    Mat bordered_source;
    copyMakeBorder(image_in, bordered_source, border, border, border, border, BORDER_CONSTANT,Scalar());
    Point2f src_center(origin.x, origin.y);
    Mat rot_mat = getRotationMatrix2D(src_center, angle, 1.0);
    Mat dst;
    warpAffine(bordered_source, dst, rot_mat, bordered_source.size());  
    return dst;
}

Mat translate_image(Mat& image_in, const int x_diff, const int y_diff)
{
	const int width_const = image_in.cols;
	const int height_const = image_in.rows;
	const int width_const_minus = width_const - 1;
	const int height_const_minus = height_const - 1;

	Mat image_result = Mat::zeros(height_const, width_const, CV_8UC1);

	for (int i = 0; i < width_const; ++i)
		for (int j = 0; j < height_const; ++j)
			if (image_in.ptr<uchar>(j, i)[0] > 0)
			{
				const int i_new = i + x_diff;
				const int j_new = j + y_diff;

				if (i_new < 0 || i_new > width_const_minus || j_new < 0 || j_new > height_const_minus)
					continue;

				image_result.ptr<uchar>(j_new, i_new)[0] = image_in.ptr<uchar>(j, i)[0];
			}

	return image_result;
}

Mat resize_image(Mat& image_in, const float scale)
{
	Mat image_resized;
	resize(image_in, image_resized, Size(image_in.cols * scale, image_in.rows * scale), 0, 0, INTER_LINEAR);

	Mat image_new = Mat::zeros(image_in.rows, image_in.cols, CV_8UC1);

	const int x_offset = (image_new.cols - image_resized.cols) / 2;
	const int y_offset = (image_new.rows - image_resized.rows) / 2;

	const int i_max = image_resized.cols;
	const int j_max = image_resized.rows;
	for (int i = 0; i < i_max; ++i)
	{
		const int i_new = i + x_offset;
		if (i_new >= image_in.cols || i_new < 0)
			continue;

		for (int j = 0; j < j_max; ++j)
		{
			const int j_new = j + y_offset;
			if (j_new >= image_in.rows || j_new < 0)
				continue;

			image_new.ptr<uchar>(j_new, i_new)[0] = image_resized.ptr<uchar>(j, i)[0];
		}
	}

	return image_new;
}

void distance_transform(Mat& image_in, float& dist_min, float& dist_max, Point& pt_dist_min, Point& pt_dist_max)
{
	Mat image_find_contours = image_in.clone();

	// vector<Vec4i> hiearchy;
	// vector<vector<Point>> contours;
	// findContours(image_find_contours, contours, hiearchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));
	vector<vector<Point>> contours = legacyFindContours(image_find_contours);

	dist_min = 9999;
	dist_max = 0;

	const int image_width_const = image_in.cols;
	const int image_height_const = image_in.rows;

	for (int i = 0; i < image_width_const; ++i)
		for (int j = 0; j < image_height_const; ++j)
			if (image_in.ptr<uchar>(j, i)[0] > 0)
			{
				float dist_min_current = 9999;
				for (vector<Point>& contour : contours)
					for (Point& pt : contour)
					{
						const float dist_current = get_distance(i, j, pt.x, pt.y);
						if (dist_current < dist_min_current)
							dist_min_current = dist_current;
					}

				if (dist_min_current < dist_min)
				{
					dist_min = dist_min_current;
					pt_dist_min = Point(i, j);
				}

				if (dist_min_current > dist_max)
				{
					dist_max = dist_min_current;
					pt_dist_max = Point(i, j);
				}
			}
}

bool compute_channel_diff_image(Mat& image_in, Mat& image_out, bool normalize, string name, bool set_norm_range, bool low_pass)
{
	bool result = true;

	static string channel_diff_image_primary_name = "";
	if (channel_diff_image_primary_name == "")
		channel_diff_image_primary_name = name;

	const int image_width_const = image_in.cols;
	const int image_height_const = image_in.rows;

	image_out = Mat(image_height_const, image_width_const, CV_8UC1);

	static uchar gray_min;
	static uchar gray_max;
	static bool do_normalize = false;

	if (name == channel_diff_image_primary_name && set_norm_range)
	{
		vector<uchar> gray_vec;
		for (int i = 0; i < image_width_const; ++i)
			for (int j = 0; j < image_height_const; ++j)
			{
				int diff0 = image_in.ptr<uchar>(j, i)[0] - image_in.ptr<uchar>(j, i)[1];
				int diff1 = image_in.ptr<uchar>(j, i)[2] - image_in.ptr<uchar>(j, i)[1];

				if (diff0 < 0)
					diff0 = 0;
				if (diff1 < 0)
					diff1 = 0;

				const uchar gray = min(diff0, diff1);
				gray_vec.push_back(gray);
				image_out.ptr<uchar>(j, i)[0] = gray;
			}

		sort(gray_vec.begin(), gray_vec.end());
		uchar gray_min_new = gray_vec[0];
		uchar gray_max_new = gray_vec[gray_vec.size() - 1];

		if (low_pass)
		{
			mat_functions_low_pass_filter.compute(gray_min_new, 0.5, "gray_min_new");
			mat_functions_low_pass_filter.compute(gray_max_new, 0.5, "gray_max_new");
		}

		if (abs(gray_min_new - gray_min) + abs(gray_max_new - gray_max) > 2)
			result = false;

		gray_min = gray_min_new;
		gray_max = gray_max_new;
		do_normalize = true;
	}
	else
	{
		for (int i = 0; i < image_width_const; ++i)
			for (int j = 0; j < image_height_const; ++j)
			{
				int diff0 = image_in.ptr<uchar>(j, i)[0] - image_in.ptr<uchar>(j, i)[1];
				int diff1 = image_in.ptr<uchar>(j, i)[2] - image_in.ptr<uchar>(j, i)[1];

				if (diff0 < 0)
					diff0 = 0;
				if (diff1 < 0)
					diff1 = 0;

				const uchar gray = min(diff0, diff1);
				image_out.ptr<uchar>(j, i)[0] = gray;
			}
	}

	if (normalize && do_normalize)
		for (int i = 0; i < image_width_const; ++i)
			for (int j = 0; j < image_height_const; ++j)
			{
				int gray = map_val(image_out.ptr<uchar>(j, i)[0], gray_min, gray_max, 0, 254);
				if (gray > 254)
					gray = 254;
				if (gray < 0)
					gray = 0;

				image_out.ptr<uchar>(j, i)[0] = gray;
			}

	rectangle(image_out, Rect(0, 0, image_in.cols, image_in.rows), Scalar(0), 2);
	return result;
}

void compute_max_image(Mat& image_in, Mat& image_out)
{
	const int image_width_const = image_in.cols;
	const int image_height_const = image_in.rows;

	for (int i = 0; i < image_width_const; ++i)
		for (int j = 0; j < image_height_const; ++j)
			image_out.ptr<uchar>(j, i)[0] = max(image_in.ptr<uchar>(j, i)[0],
										    max(image_in.ptr<uchar>(j, i)[1], image_in.ptr<uchar>(j, i)[2]));
}

void compute_active_light_image(Mat& image_regular, Mat& image_channel_diff, Mat& image_out)
{
	const int image_width_const = image_regular.cols;
	const int image_height_const = image_regular.rows;

	image_out = Mat(image_height_const, image_width_const, CV_8UC1);
	for (int i = 0; i < image_width_const; ++i)
		for (int j = 0; j < image_height_const; ++j)
		{
			const uchar gray0 = (image_regular.ptr<uchar>(j, i)[0] +
								 image_regular.ptr<uchar>(j, i)[1] +
								 image_regular.ptr<uchar>(j, i)[2]) / 3;

			const uchar gray1 = image_channel_diff.ptr<uchar>(j, i)[0];
			int gray_subtraction = abs(gray0 - gray1);
			if (gray_subtraction == 255)
				gray_subtraction = 254;

			image_out.ptr<uchar>(j, i)[0] = gray_subtraction;
		}
}

void compute_color_segmented_image(Mat& image_in, Mat& image_out)
{
	const int width = image_in.cols;
	const int height = image_in.rows;
	const int reduce_size = 16;

	Mat image_smoothed;
	GaussianBlur(image_in, image_smoothed, Size(9, 9), 0, 0);

	Mat image_reduced = Mat(height, width, CV_8UC3);

	unordered_map<string, vector<Point>> bgr_gray_index;

	for (int i = 0; i < width; ++i)
		for (int j = 0; j < height; ++j)
		{
			uchar b = image_smoothed.ptr<uchar>(j, i)[0] / reduce_size * reduce_size;
			uchar g = image_smoothed.ptr<uchar>(j, i)[1] / reduce_size * reduce_size;
			uchar r = image_smoothed.ptr<uchar>(j, i)[2] / reduce_size * reduce_size;

			image_reduced.ptr<uchar>(j, i)[0] = b;
			image_reduced.ptr<uchar>(j, i)[1] = g;
			image_reduced.ptr<uchar>(j, i)[2] = r;

			string key = to_string(b) + "," + to_string(g) + "," + to_string(r);
			bgr_gray_index[key].push_back(Point(i, j));
		}

	Mat image_segmented = Mat(height, width, CV_8UC1);

	uchar gray_segmented = 0;
	for (auto iterator = bgr_gray_index.begin(); iterator != bgr_gray_index.end(); ++iterator)
	{
		for (Point& pt : iterator->second)
			image_segmented.ptr<uchar>(pt.y, pt.x)[0] = gray_segmented;

		gray_segmented += 10;
	}

	image_out = image_segmented;
}

void compute_motion_structure_image(Mat& image_in, Mat& image_out, string name)
{
	Mat image_old = mat_functions_value_store.get_mat("image_motion_structure" + name, true);

	Mat image_subtraction = Mat(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
	for (int i = 0; i < WIDTH_SMALL; ++i)
		for (int j = 0; j < HEIGHT_SMALL; ++j)
		{
			int diff = image_in.ptr<uchar>(j, i)[0] - image_old.ptr<uchar>(j, i)[0] + 127;
			if (diff < 0)
				diff = 0;
			else if (diff > 254)
				diff = 254;

			image_subtraction.ptr<uchar>(j, i)[0] = diff;
		}
	equalizeHist(image_subtraction, image_subtraction);

	mat_functions_value_store.set_mat("image_motion_structure" + name, image_in);
	image_out = image_subtraction;
}

void print_mat_type(Mat& image_in)
{
	int type = image_in.type();

	string r;

	uchar depth = type & CV_MAT_DEPTH_MASK;
	uchar chans = 1 + (type >> CV_CN_SHIFT);

	switch (depth)
	{
		case CV_8U:  r = "8U"; break;
		case CV_8S:  r = "8S"; break;
		case CV_16U: r = "16U"; break;
		case CV_16S: r = "16S"; break;
		case CV_32S: r = "32S"; break;
		case CV_32F: r = "32F"; break;
		case CV_64F: r = "64F"; break;
		default:     r = "User"; break;
	}

	r += "C";
	r += (chans + '0');

	console_log(r);
}

void put_text(string text, Mat& img, int x, int y)
{
	int fontFace = FONT_HERSHEY_SCRIPT_SIMPLEX;
	double fontScale = 0.5;
	cv::Point textOrg(x, y);
	cv::putText(img, text, textOrg, fontFace, fontScale, Scalar::all(0), 3, 8);
	cv::putText(img, text, textOrg, fontFace, fontScale, Scalar::all(255), 1, 8);
}