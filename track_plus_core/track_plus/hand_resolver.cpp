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

#include "hand_resolver.h"

void HandResolver::compute(MonoProcessorNew& mono_processor0,     MonoProcessorNew& mono_processor1,
						   MotionProcessorNew& motion_processor0, MotionProcessorNew& motion_processor1,
						   Mat& image0, 					      Mat& image1,
						   Reprojector& reprojector,              bool visualize)
{
	Mat image_visualization0;
	Mat image_visualization1;

	if (visualize)
	{
		image_visualization0 = reprojector.remap(&image0, 0, true);
		image_visualization1 = reprojector.remap(&image1, 1, true);
	}

	pt_precise_palm0.x = mono_processor0.pt_palm.x * 4;
	pt_precise_palm0.y = HEIGHT_LARGE_MINUS - (mono_processor0.pt_palm.y * 4);
	if (pt_precise_palm0.y < 0)
		pt_precise_palm0.y = 0;

	pt_precise_palm1.x = mono_processor1.pt_palm.x * 4;
	pt_precise_palm1.y = HEIGHT_LARGE_MINUS - (mono_processor1.pt_palm.y * 4);
	if (pt_precise_palm1.y < 0)
		pt_precise_palm1.y = 0;

	pt_precise_index0 = Point2f(-1, -1);
	if (mono_processor0.pt_index.x != -1)
		pt_precise_index0 = increase_resolution(mono_processor0.pt_index,                  image0,
											    motion_processor0.image_background_static, motion_processor0.diff_threshold,
											    motion_processor0.gray_threshold_left,     motion_processor0.gray_threshold_right,
											    reprojector,                               0,
											    										   motion_processor0.x_separator_middle_median);

	pt_precise_index1 = Point2f(-1, -1);
	if (mono_processor1.pt_index.x != -1)
		pt_precise_index1 = increase_resolution(mono_processor1.pt_index,                  image1,
											    motion_processor1.image_background_static, motion_processor1.diff_threshold,
											    motion_processor1.gray_threshold_left,     motion_processor1.gray_threshold_right,
											    reprojector,                               1,
											    										   motion_processor1.x_separator_middle_median);

	pt_precise_thumb0 = Point2f(-1, -1);
	if (mono_processor0.pt_thumb.x != -1)
		pt_precise_thumb0 = increase_resolution(mono_processor0.pt_thumb,                  image0,
												motion_processor0.image_background_static, motion_processor0.diff_threshold,
												motion_processor0.gray_threshold_left,     motion_processor0.gray_threshold_right,
												reprojector,                               0,
												                                           motion_processor1.x_separator_middle_median);

	pt_precise_thumb1 = Point2f(-1, -1);
	if (mono_processor1.pt_thumb.x != -1)
		pt_precise_thumb1 = increase_resolution(mono_processor1.pt_thumb,                  image1,
											    motion_processor1.image_background_static, motion_processor1.diff_threshold,
											    motion_processor1.gray_threshold_left,     motion_processor1.gray_threshold_right,
											    reprojector,                               1,
											    									       motion_processor1.x_separator_middle_median);

	if (pt_precise_index0.x == -1 || pt_precise_index1.x == -1)
	{
		pt_precise_index0 = Point2f(-1, -1);
		pt_precise_index1 = Point2f(-1, -1);
	}
	else if (visualize)
	{
		circle(image_visualization0, pt_precise_index0, 5, Scalar(255), -1);
		circle(image_visualization1, pt_precise_index1, 5, Scalar(255), -1);
	}

	if (pt_precise_thumb0.x == -1 || pt_precise_thumb1.x == -1)
	{
		pt_precise_thumb0 = Point2f(-1, -1);
		pt_precise_thumb1 = Point2f(-1, -1);
	}
	else if (visualize)
	{
		circle(image_visualization0, pt_precise_thumb0, 5, Scalar(255), -1);
		circle(image_visualization1, pt_precise_thumb1, 5, Scalar(255), -1);
	}

	if (visualize)
	{
		imshow("image_visualization0", image_visualization0);
		imshow("image_visualization1", image_visualization1);
	}
}

Point2f HandResolver::increase_resolution(Point& pt_in,                    Mat& image_in,
										  Mat& image_background_in,        const uchar diff_threshold,
										  const uchar gray_threshold_left, const uchar gray_threshold_right,
										  Reprojector& reprojector,        const uchar side,
										  							       const int x_separator_middle_median)
{
	Point pt_large = pt_in * 4;

	int x0 = pt_large.x - window_width_half;
	int y0 = pt_large.y - window_height_half;
	int x1 = pt_large.x + window_width_half;
	int y1 = pt_large.y + window_height_half;

	if (x0 < 0)
		x0 = 0;
	if (y0 < 0)
		y0 = 0;
	if (x1 > WIDTH_LARGE_MINUS)
		x1 = WIDTH_LARGE_MINUS;
	if (y1 > HEIGHT_LARGE_MINUS)
		y1 = HEIGHT_LARGE_MINUS;

	Rect crop_rect = Rect(x0, y0, x1 - x0, y1 - y0);

	int crop_rect_small_width = (x1 - x0) / 4;
	if (crop_rect_small_width > WIDTH_SMALL_MINUS)
		crop_rect_small_width = WIDTH_SMALL_MINUS;

	int crop_rect_small_height = (y1 - y0) / 4;
	if (crop_rect_small_height > HEIGHT_SMALL_MINUS)
		crop_rect_small_height = HEIGHT_SMALL_MINUS;

	Rect crop_rect_small = Rect(x0 / 4, y0 / 4, crop_rect_small_width, crop_rect_small_height);

	Mat image_cropped = image_in(crop_rect);
	GaussianBlur(image_cropped, image_cropped, Size(21, 21), 0, 0);

	Mat image_cropped_preprocessed;
	compute_channel_diff_image(image_cropped, image_cropped_preprocessed, true, "image_cropped_preprocessed");
	Point pt_offset0;
	image_cropped_preprocessed = reprojector.remap(&image_cropped_preprocessed, x0, y0, side, pt_offset0);

	Mat image_background_cropped = image_background_in(crop_rect_small).clone();

	const int image_background_cropped_width = image_background_cropped.cols;
	const int image_background_cropped_height = image_background_cropped.rows;

	uchar gray_max = 0;
	for (int i = 0; i < image_background_cropped_width; ++i)
		for (int j = 0; j < image_background_cropped_height; ++j)
		{
			const uchar gray = image_background_cropped.ptr<uchar>(j, i)[0];
			if (gray < 255 && gray > gray_max)
				gray_max = gray;
		}

	for (int i = 0; i < image_background_cropped_width; ++i)
		for (int j = 0; j < image_background_cropped_height; ++j)
			if (image_background_cropped.ptr<uchar>(j, i)[0] == 255)
				image_background_cropped.ptr<uchar>(j, i)[0] = gray_max;

	Mat image_background_cropped_scaled;
	resize(image_background_cropped, image_background_cropped_scaled, crop_rect.size(), 0, 0, INTER_LINEAR);

	Point pt_offset1;
	image_background_cropped_scaled = reprojector.remap(&image_background_cropped_scaled, x0, y0, side, pt_offset1);

	if (image_background_cropped_scaled.cols == 0)
		return Point(-1, -1);

	const int image_width = image_cropped_preprocessed.cols;
	const int image_height = image_cropped_preprocessed.rows;

	Mat image_subtraction = Mat::zeros(image_height, image_width, CV_8UC1);
	for (int i = 0; i < image_width; ++i)
	{
		if (i <= x_separator_middle_median)
		{
			for (int j = 0; j < image_height; ++j)
				if (image_cropped_preprocessed.ptr<uchar>(j, i)[0] > gray_threshold_left)
					image_subtraction.ptr<uchar>(j, i)[0] = abs(image_cropped_preprocessed.ptr<uchar>(j, i)[0] - 
															    image_background_cropped_scaled.ptr<uchar>(j, i)[0]);
		}
		else
		{
			for (int j = 0; j < image_height; ++j)
				if (image_cropped_preprocessed.ptr<uchar>(j, i)[0] > gray_threshold_right)
					image_subtraction.ptr<uchar>(j, i)[0] = abs(image_cropped_preprocessed.ptr<uchar>(j, i)[0] - 
															    image_background_cropped_scaled.ptr<uchar>(j, i)[0]);
		}
	}

	threshold(image_subtraction, image_subtraction, diff_threshold, 254, THRESH_BINARY);
	blob_detector_image_subtraction.compute(image_subtraction, 254, 0, image_width, 0, image_height, true);

	if (blob_detector_image_subtraction.blobs->size() == 0)
		return Point(-1, -1);

	float x_mean = 0;
	float y_mean = 0;
	int count = 0;

	for (Point& pt : blob_detector_image_subtraction.blob_max_size->data)
	{
		x_mean += pt.x;
		y_mean += pt.y;
		++count;
	}

	x_mean /= count;
	y_mean /= count;

	return Point2f(x_mean + pt_offset0.x, y_mean + pt_offset0.y);
}